/*
 * descripten - ECMAScript to native compiler
 * Copyright (C) 2011-2014 Christian Kindahl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <cstdarg>
#include <sstream>
#include <string.h>
#include <gc_cpp.h>
#include "config.hh"
#include "common/cast.hh"
#include "common/conversion.hh"
#include "common/exception.hh"
#include "algorithm.hh"
#include "context.hh"
#include "conversion.hh"
#include "debug.hh"
#include "environment.hh"
#include "error.hh"
#include "frame.hh"
#include "global.hh"
#include "messages.hh"
#include "native.hh"
#include "operation.hh"
#include "property.hh"
#include "standard.hh"
#include "utility.hh"
#include "value.hh"

#ifdef PROFILE
#include "profiler.hh"
#endif  // PROFILE

class EsPropertyIterator
{
private:
    EsObject *obj_;
    EsObject::Iterator it_cur_;
    EsObject::Iterator it_end_;

public:
    EsPropertyIterator(EsObject *obj)
        : obj_(obj)
    {
        it_cur_ = obj->begin_recursive();
        it_end_ = obj->end_recursive();
    }

    bool next(EsValue &v)
    {
        for (; it_cur_ != it_end_; ++it_cur_)
        {
            EsPropertyKey key = *it_cur_;

            EsPropertyReference prop = obj_->get_property(key);
            if (!prop || !prop->is_enumerable())    // The property might have been deleted.
                continue;

            it_cur_++;
            v = EsValue::from_str(key.to_string());
            return true;
        }

        return false;
    }
};

void data_reg_str(const String &str, uint32_t id)
{
    strings().unsafe_intern(str, id);
}

void op_stk_alloc(size_t count)
{
    g_call_stack.alloc(count);
}

void op_stk_free(size_t count)
{
    g_call_stack.free(count);
}

void op_stk_push(const EsValue &val)
{
    g_call_stack.push(val);
}

void op_init_args(EsValue dst[], int argc, const EsValue argv[], int prmc)
{
    int i = 0;
    if (prmc <= argc)
    {
        for (; i < prmc; i++)
            dst[i] = argv[i];
    }
    else
    {
        for (; i < argc; i++)
            dst[i] = argv[i];

        for (; i < prmc; i++)
            dst[i] = EsValue::undefined;
    }
}

EsValue op_args_obj_init(EsContext *ctx, int argc,
                         EsValue *fp, EsValue *vp)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    assert(ctx->var_env()->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env =
        static_cast<EsDeclarativeEnvironmentRecord *>(
            ctx->var_env()->env_rec());

    if (!env->has_binding(property_keys.arguments))
    {
        EsArguments *args_obj = EsArguments::create_inst(
            frame.callee().as_function(), argc, fp);
        if (ctx->is_strict())
        {
            env->create_immutable_binding(property_keys.arguments,
                                          EsValue::from_obj(args_obj));
        }
        else
        {
            env->create_mutable_binding(property_keys.arguments, false);
            env->set_mutable_binding(property_keys.arguments,
                                     EsValue::from_obj(args_obj));
        }

        return EsValue::from_obj(args_obj);
    }

    return EsValue::nothing;
}

void op_args_obj_link(EsValue &args, int i, EsValue *val)
{
    assert(args.is_object());

    EsArguments *args_obj = safe_cast<EsArguments *>(args.as_object());
    args_obj->link_parameter(i, val);
}

EsValue *op_bnd_extra_init(EsContext *ctx, int num_extra)
{
    EsValue *extra = new (GC)EsValue[num_extra];
    for (int i = 0; i < num_extra; i++)
        extra[i] = EsValue::undefined;

    assert(ctx->var_env()->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env =
        static_cast<EsDeclarativeEnvironmentRecord *>(
            ctx->var_env()->env_rec());

    env->set_storage(extra, num_extra);
    return extra;
}

EsValue *op_bnd_extra_ptr(int argc, EsValue *fp, EsValue *vp, int hops)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsLexicalEnvironment *env = frame.callee().as_function()->scope();
    for (int i = 1; i < hops; i++)
        env = env->outer();

    assert(env);
    assert(env->env_rec()->is_decl_env());
    return static_cast<EsDeclarativeEnvironmentRecord *>(
        env->env_rec())->storage();
}

#ifdef FEATURE_CONTEXT_CACHE
struct ContextLookupCacheEntry
{
    EsMap::Id id;
    EsPropertyKey key;
    EsPropertyReference prop;

    ContextLookupCacheEntry()
        : id()
    {
    }
};

ContextLookupCacheEntry context_cache[FEATURE_CONTEXT_CACHE_SIZE];
#endif  // FEATURE_CONTEXT_CACHE

bool ctx_cached_getT(EsObject *obj, EsPropertyKey key,
                     EsPropertyReference &prop, uint16_t cid)
{
#ifdef FEATURE_CONTEXT_CACHE
    assert(cid < FEATURE_CONTEXT_CACHE_SIZE);

#ifdef PROFILE
    profiler::stats.ctx_access_cnt_++;
#endif  // PROFILE

    ContextLookupCacheEntry &cache_entry = context_cache[cid];

    // We only allow caching of the global object because this implementation
    // is not capable of caching an object hierarchy which might be the case
    // with "with" objects.
    if (obj == es_global_obj())
    {
        if (cache_entry.id == obj->map().id() &&
            cache_entry.key == key)
        {
#ifdef PROFILE
            profiler::stats.ctx_cache_hits_++;
#endif  // PROFILE

            prop = obj->map().from_cached(cache_entry.prop);
            return true;
        }
    }

#ifdef PROFILE
    profiler::stats.ctx_cache_misses_++;
#endif  // PROFILE
#endif  // FEATURE_CONTEXT_CACHE

    if (!obj->getT(key, prop))
        return false;

#ifdef FEATURE_CONTEXT_CACHE
    if (!prop || !prop.is_cachable())
    {
        cache_entry.id = EsMap::Id();
        return true;
    }

    cache_entry.id = obj->map().id();
    cache_entry.key = key;
    cache_entry.prop = prop;
#endif  // FEATURE_CONTEXT_CACHE

    return true;
}

EsPropertyReference ctx_cached_get_property(EsObject *obj,
                                            EsPropertyKey key,
                                            uint16_t cid)
{
#ifdef FEATURE_CONTEXT_CACHE
    assert(cid < FEATURE_CONTEXT_CACHE_SIZE);

#ifdef PROFILE
    profiler::stats.ctx_access_cnt_++;
#endif  // PROFILE

    ContextLookupCacheEntry &cache_entry = context_cache[cid];

    // We only allow caching of the global object because this implementation
    // is not capable of caching an object hierarchy which might be the case
    // with "with" objects.
    if (obj == es_global_obj())
    {
        if (cache_entry.id == obj->map().id() &&
            cache_entry.key == key)
        {
#ifdef PROFILE
            profiler::stats.ctx_cache_hits_++;
#endif  // PROFILE

            return obj->map().from_cached(cache_entry.prop);
        }
    }

#ifdef PROFILE
    profiler::stats.ctx_cache_misses_++;
#endif  // PROFILE
#endif  // FEATURE_CONTEXT_CACHE

    EsPropertyReference prop = obj->get_property(key);

#ifdef FEATURE_CONTEXT_CACHE
    if (!prop || !prop.is_cachable())
    {
        cache_entry.id = EsMap::Id();
        return prop;
    }

    cache_entry.id = obj->map().id();
    cache_entry.key = key;
    cache_entry.prop = prop;
#endif  // FEATURE_CONTEXT_CACHE

    return prop;
}

EsPropertyReference ctx_cached_get_own_property(EsObject *obj,
                                                EsPropertyKey key,
                                                uint16_t cid)
{
#ifdef FEATURE_CONTEXT_CACHE
    assert(cid < FEATURE_CONTEXT_CACHE_SIZE);

#ifdef PROFILE
    profiler::stats.ctx_access_cnt_++;
#endif  // PROFILE

    ContextLookupCacheEntry &cache_entry = context_cache[cid];

    // We only allow caching of the global object because this implementation
    // is not capable of caching an object hierarchy which might be the case
    // with "with" objects.
    if (obj == es_global_obj())
    {
        if (cache_entry.id == obj->map().id() &&
            cache_entry.key == key)
        {
#ifdef PROFILE
            profiler::stats.ctx_cache_hits_++;
#endif  // PROFILE

            return obj->map().from_cached(cache_entry.prop);
        }
    }

#ifdef PROFILE
    profiler::stats.ctx_cache_misses_++;
#endif  // PROFILE
#endif  // FEATURE_CONTEXT_CACHE

    EsPropertyReference prop = obj->get_own_property(key);

#ifdef FEATURE_CONTEXT_CACHE
    if (!prop || !prop.is_cachable())
    {
        cache_entry.id = EsMap::Id();
        return prop;
    }

    cache_entry.id = obj->map().id();
    cache_entry.key = key;
    cache_entry.prop = prop;
#endif  // FEATURE_CONTEXT_CACHE

    return prop;
}

bool op_ctx_get(EsContext *ctx, uint64_t raw_key, EsValue &result,
                uint16_t cid)
{
    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    for (auto lex = ctx->lex_env(); lex; lex = lex->outer())
    {
        EsEnvironmentRecord *env_rec = lex->env_rec();
        if (env_rec->is_obj_env())
        {
            EsObjectEnvironmentRecord *env =
                safe_cast<EsObjectEnvironmentRecord *>(env_rec);

            EsObject *obj = env->binding_object();

            EsPropertyReference prop;
            if (!ctx_cached_getT(obj, key, prop, cid))
            {
                assert(false);
                op_ex_clear(ctx);
                return false;
            }

            if (!prop)
                continue;

            return obj->get_resolveT(prop, result);
        }
        else
        {
            EsDeclarativeEnvironmentRecord *env =
                safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

            // FIXME: These two calls should be combined somehow.
            if (env->has_binding(key))
                return env->get_binding_valueT(key, ctx->is_strict(), result);
        }
    }

    ES_THROW(EsReferenceError,
             es_fmt_msg(ES_MSG_REF_NOT_DEFINED,
                        key.to_string().utf8().c_str()));
    return false;
}

bool op_ctx_put(EsContext *ctx, uint64_t raw_key, const EsValue &val,
                uint16_t cid)
{
    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    for (auto lex = ctx->lex_env(); lex; lex = lex->outer())
    {
        EsEnvironmentRecord *env_rec = lex->env_rec();
        if (env_rec->is_obj_env())
        {
            EsObjectEnvironmentRecord *env =
                safe_cast<EsObjectEnvironmentRecord *>(env_rec);

            EsObject *obj = env->binding_object();

            EsPropertyReference prop = ctx_cached_get_own_property(obj, key, cid);
            if (prop)
                return obj->put_ownT(key, prop, val, ctx->is_strict());

            if (obj->has_property(key))
                return obj->putT(key, val, ctx->is_strict());
        }
        else
        {
            EsDeclarativeEnvironmentRecord *env =
                safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

            if (env->has_binding(key))
                return env->set_mutable_bindingT(key, val, ctx->is_strict());
        }
    }

    if (ctx->is_strict())
    {
        ES_THROW(EsReferenceError,
                 es_fmt_msg(ES_MSG_REF_UNRESOLVABLE,
                            key.to_string().utf8().c_str()));
        return false;
    }

    return es_global_obj()->putT(key, val, false);
}

bool op_ctx_del(EsContext *ctx, uint64_t raw_key, EsValue &result)
{
    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    for (auto lex = ctx->lex_env(); lex; lex = lex->outer())
    {
        EsEnvironmentRecord *env_rec = lex->env_rec();

        if (env_rec->has_binding(key))
        {
            bool removed = false;

            if (env_rec->is_obj_env())
            {
                EsObjectEnvironmentRecord *env =
                    safe_cast<EsObjectEnvironmentRecord *>(env_rec);

                if (!env->binding_object()->removeT(key, false, removed))
                    return false;
            }
            else
            {
                EsDeclarativeEnvironmentRecord *env =
                    safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

                if (!env->delete_bindingT(key, removed))
                    return false;
            }

            result = EsValue::from_bool(removed);
            return true;
        }
    }

    if (ctx->is_strict())
    {
        ES_THROW(EsSyntaxError, es_fmt_msg(ES_MSG_SYNTAX_DELETE_UNRESOLVABLE));
        return false;
    }

    result = EsValue::from_bool(true);
    return true;
}

void op_ctx_set_strict(EsContext *ctx, bool strict)
{
    ctx->set_strict(strict);
}

bool op_ctx_enter_with(EsContext *ctx, const EsValue &val)
{
    return EsContextStack::instance().push_withT(val);
}

bool op_ctx_enter_catch(EsContext *ctx, uint64_t raw_key)
{
    assert(ctx->has_pending_exception());

    EsContextStack::instance().push_catch(EsPropertyKey::from_raw(raw_key),
                                          ctx->get_pending_exception());

    ctx->clear_pending_exception();
    return true;    // FIXME:
}

void op_ctx_leave()
{
    EsContextStack::instance().pop();
}

EsContext *op_ctx_running()
{
    return EsContextStack::instance().top();
}

EsValue op_ex_save_state(EsContext *ctx)
{
    return ctx->get_pending_exception();
}

void op_ex_load_state(EsContext *ctx, const EsValue &state)
{
    ctx->set_pending_exception(state);
}

void op_ex_set(EsContext *ctx, const EsValue &exception)
{
    ctx->set_pending_exception(exception);
    assert(ctx->has_pending_exception());
}

void op_ex_clear(EsContext *ctx)
{
    ctx->clear_pending_exception();
}

bool op_ctx_decl_fun(EsContext *ctx, bool is_eval, bool is_strict,
                     uint64_t fn, const EsValue &fo)
{
    EsPropertyKey fn_key = EsPropertyKey::from_raw(fn);

    EsEnvironmentRecord *env_rec = ctx->var_env()->env_rec();

    // 10.5:5
    bool fun_already_declared = env_rec->has_binding(fn_key);
    if (!fun_already_declared)
    {
        // FIXME: We should have two different ctx_decl_fun for object and
        //        declarative environment records.
        if (env_rec->is_obj_env())
        {
            EsObjectEnvironmentRecord *env =
                safe_cast<EsObjectEnvironmentRecord *>(env_rec);

            return env->binding_object()->define_own_propertyT(fn_key,
                EsPropertyDescriptor(true, is_eval, true, fo), true);   // VERIFIED: 10.2.1.2.2
        }
        else
        {
            EsDeclarativeEnvironmentRecord *env =
                safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

            env->create_mutable_binding(fn_key, is_eval);

            return env->set_mutable_bindingT(fn_key, fo, is_strict);
        }
    }
    else if (env_rec == es_global_env()->env_rec())
    {
        EsObject *go = es_global_obj();
        EsPropertyReference existing_prop = go->get_property(fn_key);
        if (existing_prop->is_configurable())
        {
            return go->define_own_propertyT(fn_key,
                EsPropertyDescriptor(true, is_eval, true, fo), true);
        }
        else if (existing_prop->is_accessor() ||
                 (!existing_prop->is_writable() &&
                  !existing_prop->is_enumerable()))
        {
            ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_DECL, fn_key.to_string().utf8().c_str()));
            return false;
        }

        return go->putT(fn_key, fo, is_strict);
    }

    if (env_rec->is_obj_env())
    {
        EsObjectEnvironmentRecord *env =
            safe_cast<EsObjectEnvironmentRecord *>(env_rec);

        return env->binding_object()->putT(fn_key, fo, is_strict);
    }
    else
    {
        EsDeclarativeEnvironmentRecord *env =
            safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

        return env->set_mutable_bindingT(fn_key, fo, is_strict);
    }

    assert(false);
    return true;
}

bool op_ctx_decl_var(EsContext *ctx, bool is_eval, bool is_strict,
                     uint64_t vn)
{
    EsPropertyKey vn_key = EsPropertyKey::from_raw(vn);

    EsEnvironmentRecord *env_rec = ctx->var_env()->env_rec();

    // 10.5:8
    bool var_already_declared = env_rec->has_binding(vn_key);
    if (!var_already_declared)
    {
        if (env_rec->is_obj_env())
        {
            EsObjectEnvironmentRecord *env =
                safe_cast<EsObjectEnvironmentRecord *>(env_rec);

            return env->binding_object()->define_own_propertyT(vn_key,
                EsPropertyDescriptor(true, is_eval, true,
                                     EsValue::undefined), true);    // VERIFIED: 10.2.1.2.2
        }
        else
        {
            EsDeclarativeEnvironmentRecord *env =
                safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

            env->create_mutable_binding(vn_key, is_eval);

            return env->set_mutable_bindingT(vn_key, EsValue::undefined, is_strict);
        }
    }

    return true;
}

bool op_ctx_decl_prm(EsContext *ctx, bool is_strict, uint64_t pn,
                     const EsValue &po)
{
    EsPropertyKey pn_key = EsPropertyKey::from_raw(pn);

    assert(ctx->var_env()->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env_rec =
        static_cast<EsDeclarativeEnvironmentRecord *>(
            ctx->var_env()->env_rec());

    if (!env_rec->has_binding(pn_key))
        env_rec->create_mutable_binding(pn_key, false);

    return env_rec->set_mutable_bindingT(pn_key, po, is_strict);
}

void op_ctx_link_fun(EsContext *ctx, uint64_t fn, EsValue *fo)
{
    EsPropertyKey fn_key = EsPropertyKey::from_raw(fn);

    assert(ctx->var_env()->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env =
        static_cast<EsDeclarativeEnvironmentRecord *>(
            ctx->var_env()->env_rec());

    // Note: Don't inherit existing binding values since fo has been
    //       initialized with a function object.
    env->link_mutable_binding(fn_key, false, fo, false);
}

void op_ctx_link_var(EsContext *ctx, uint64_t vn, EsValue *vo)
{
    EsPropertyKey vn_key = EsPropertyKey::from_raw(vn);

    assert(ctx->var_env()->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env =
        static_cast<EsDeclarativeEnvironmentRecord *>(
            ctx->var_env()->env_rec());

    // Note: Inherit existing binding values since the arguments object might
    //       be overridden. The arguments binding is bound when creating the
    //       execution context, before calling this function.
    env->link_mutable_binding(vn_key, false, vo, true);
}

void op_ctx_link_prm(EsContext *ctx, uint64_t pn, EsValue *po)
{
    EsPropertyKey pn_key = EsPropertyKey::from_raw(pn);

    assert(ctx->var_env()->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env =
        static_cast<EsDeclarativeEnvironmentRecord *>(
            ctx->var_env()->env_rec());

    env->link_mutable_binding(pn_key, false, po, true);
}

EsPropertyIterator *op_prp_it_new(const EsValue &val)
{
    EsObject *obj = val.to_objectT();
    if (!obj)
        return NULL;

    EsPropertyIterator *it = new (GC)EsPropertyIterator(obj);
    return it;
}

bool op_prp_it_next(EsPropertyIterator *it, EsValue &val)
{
    return it->next(val);
}

bool op_prp_def_data(EsValue &obj_val, const EsValue &key, const EsValue &val)
{
    assert(obj_val.is_object());

    String name;
    if (!key.to_string(name))
        return false;

    uint32_t index = 0;
    if (es_str_to_index(name, index))
        return obj_val.as_object()->define_own_propertyT(
            EsPropertyKey::from_u32(index), EsPropertyDescriptor(true, true, true, val), false);

    return obj_val.as_object()->define_own_propertyT(
        EsPropertyKey::from_str(name), EsPropertyDescriptor(true, true, true, val), false);
}

bool op_prp_def_accessor(EsValue &obj_val, uint64_t raw_key,
                         const EsValue &fun, bool is_setter)
{
    assert(obj_val.is_object());

    EsFunction *f = fun.as_function();

    return obj_val.as_object()->define_own_propertyT(
        EsPropertyKey::from_raw(raw_key),
        EsPropertyDescriptor(true, true,
                             is_setter ? Maybe<EsValue>() : EsValue::from_obj(f),
                             is_setter ? EsValue::from_obj(f) : Maybe<EsValue>()), false);
}

#ifdef FEATURE_PROPERTY_CACHE
struct PropertyLookupCacheEntry
{
    static const size_t max_obj_hierarchy_depth = 8;

    EsMap::Id hierarchy[max_obj_hierarchy_depth] = {};
    uint8_t hierarchy_depth = 0;
    EsPropertyKey key;
    EsPropertyReference prop;
};

PropertyLookupCacheEntry property_cache[FEATURE_PROPERTY_CACHE_SIZE];
#endif  // FEATURE_PROPERTY_CACHE

bool op_prp_get(const EsValue &obj_val, const EsValue &key_val,
                EsValue &result, uint16_t cid)
{
    uint32_t key_idx = 0;
    if (key_val.is_number() && es_num_to_index(key_val.as_number(), key_idx))
    {
        return op_prp_get(obj_val, EsPropertyKey::from_u32(key_idx).as_raw(),
                          result, cid);
    }

    String key_str;
    if (!key_val.to_string(key_str))
        return false;

    return op_prp_get(obj_val, EsPropertyKey::from_str(key_str).as_raw(), result, cid);
}

bool op_prp_get(const EsValue &obj_val, uint64_t raw_key,
                EsValue &result, uint16_t cid)
{
    EsObject *obj = obj_val.to_objectT();
    if (!obj)
        return false;

    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

#ifdef FEATURE_PROPERTY_CACHE
    assert(cid < FEATURE_PROPERTY_CACHE_SIZE);

#ifdef PROFILE
    profiler::stats.prp_access_cnt_++;
#endif  // PROFILE

    PropertyLookupCacheEntry &cache_entry = property_cache[cid];
    if (cache_entry.hierarchy_depth > 0 &&
        cache_entry.key == key)
    {
        EsObject *base_obj = obj;

        uint8_t last = cache_entry.hierarchy_depth - 1;
        for (uint8_t i = 0; i < last; i++)
        {
            if (cache_entry.hierarchy[i] != base_obj->map().id())
                goto cache_miss;

            base_obj = base_obj->prototype();
            if (!base_obj)
                goto cache_miss;
        }

        if (cache_entry.hierarchy[last] == base_obj->map().id())
        {
#ifdef PROFILE
            profiler::stats.prp_cache_hits_++;
#endif  // PROFILE

            EsPropertyReference prop =
                base_obj->map().from_cached(cache_entry.prop);

            return obj->get_resolveT(prop, result);
        }
    }

#ifdef PROFILE
    profiler::stats.prp_cache_misses_++;
#endif  // PROFILE
cache_miss:
#endif  // FEATURE_PROPERTY_CACHE

    EsPropertyReference prop;
    if (!obj->getT(key, prop))
        return false;

#ifdef FEATURE_PROPERTY_CACHE
    if (!prop || !prop.is_cachable())
        return obj->get_resolveT(prop, result);

    cache_entry.hierarchy_depth = 0;
    cache_entry.key = key;
    cache_entry.prop = prop;

    uint8_t i = 0;
    for (EsObject *base_obj = obj; base_obj; base_obj = base_obj->prototype())
    {
        cache_entry.hierarchy[i++] = base_obj->map().id();
        if (base_obj == prop.base())
            break;

        if (i >= PropertyLookupCacheEntry::max_obj_hierarchy_depth)
            return obj->get_resolveT(prop, result);
    }

    cache_entry.hierarchy_depth = i;
#endif  // FEATURE_PROPERTY_CACHE

    return obj->get_resolveT(prop, result);
}

EsPropertyReference prp_cached_get_own_property(EsObject *obj,
                                                EsPropertyKey key,
                                                uint16_t cid)
{
#ifdef FEATURE_PROPERTY_CACHE
    assert(cid < FEATURE_PROPERTY_CACHE_SIZE);

#ifdef PROFILE
    profiler::stats.prp_access_cnt_++;
#endif  // PROFILE

    PropertyLookupCacheEntry &cache_entry = property_cache[cid];
    if (cache_entry.hierarchy_depth > 0 &&
        cache_entry.key == key)
    {
        size_t last = cache_entry.hierarchy_depth - 1;
        if (cache_entry.hierarchy[last] == obj->map().id())
        {
#ifdef PROFILE
            profiler::stats.prp_cache_hits_++;
#endif  // PROFILE

            return obj->map().from_cached(cache_entry.prop);
        }
    }

#ifdef PROFILE
    profiler::stats.prp_cache_misses_++;
#endif  // PROFILE
#endif  // FEATURE_PROPERTY_CACHE

    EsPropertyReference prop = obj->get_own_property(key);

#ifdef FEATURE_PROPERTY_CACHE
    if (!prop || !prop.is_cachable())
        return prop;

    cache_entry.hierarchy_depth = 1;
    cache_entry.key = key;
    cache_entry.prop = prop;
    cache_entry.hierarchy[0] = obj->map().id();
#endif  // FEATURE_PROPERTY_CACHE

    return prop;
}

bool op_prp_put(EsContext *ctx, const EsValue &obj_val, const EsValue &key_val,
                const EsValue &val, uint16_t cid)
{
    uint32_t key_idx = 0;
    if (key_val.is_number() && es_num_to_index(key_val.as_number(), key_idx))
    {
        return op_prp_put(ctx, obj_val,
                          EsPropertyKey::from_u32(key_idx).as_raw(), val, cid);
    }

    String key_str;
    if (!key_val.to_string(key_str))
        return false;

    return op_prp_put(ctx, obj_val, EsPropertyKey::from_str(key_str).as_raw(), val, cid);
}

bool op_prp_put(EsContext *ctx, const EsValue &obj_val,
                uint64_t raw_key, const EsValue &val, uint16_t cid)
{
    EsObject *obj = obj_val.to_objectT();
    if (!obj)
        return false;

    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    EsPropertyReference prop = prp_cached_get_own_property(obj, key, cid);
    if (prop)
        return obj->put_ownT(key, prop, val, ctx->is_strict());
    return obj->putT(key, val, ctx->is_strict());
}

bool op_prp_del(EsContext *ctx, EsValue &obj_val, const EsValue &key_val,
                EsValue &result)
{
    uint32_t key_idx = 0;
    if (key_val.is_number() && es_num_to_index(key_val.as_number(), key_idx))
    {
        return op_prp_del(ctx, obj_val,
                          EsPropertyKey::from_u32(key_idx).as_raw(), result);
    }

    String key_str;
    if (!key_val.to_string(key_str))
        return false;

    return op_prp_del(ctx, obj_val, EsPropertyKey::from_str(key_str).as_raw(), result);
}

bool op_prp_del(EsContext *ctx, EsValue &obj_val, uint64_t raw_key,
                EsValue &result)
{
    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    EsObject *obj = obj_val.to_objectT();
    if (!obj)
        return false;

    bool removed = false;
    if (!obj->removeT(key, ctx->is_strict(), removed))
        return false;

    result = EsValue::from_bool(removed);
    return true;
}

bool op_call(const EsValue &fun, int argc, EsValue &result)
{
    EsCallStackGuard guard(argc);

    if (!fun.is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }

    guard.release();

    EsCallFrame frame = EsCallFrame::push_function_excl_args(
        argc, fun.as_function(), EsValue::undefined);
    if (!fun.as_function()->callT(frame))
        return false;

    result = frame.result();
    return true;
}

bool call_keyed(const EsValue &obj_val, uint64_t raw_key, int argc,
                EsValue &result)
{
    EsCallStackGuard guard(argc);

    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    EsObject *obj = obj_val.to_objectT();
    if (!obj)
        return false;

    EsValue fun_val;
    if (!obj->getT(key, fun_val))
        return false;

    if (!fun_val.is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }

    EsValue this_value = obj->implicit_this_value();

    int flags = 0;
    if (key == property_keys.eval)
        flags |= EsFunction::CALL_DIRECT_EVAL;

    guard.release();

    EsFunction *fun = fun_val.as_function();

    EsCallFrame frame = EsCallFrame::push_function_excl_args(
        argc, fun, this_value);
    if (!fun->callT(frame, flags))
        return false;

    result = frame.result();
    return true;
}

bool op_call_keyed(const EsValue &obj_val, const EsValue &key_val, int argc,
                   EsValue &result)
{
    EsCallStackGuard guard(argc);

    uint32_t key_idx = 0;
    if (key_val.is_number() && es_num_to_index(key_val.as_number(), key_idx))
    {
        guard.release();
        return call_keyed(obj_val, EsPropertyKey::from_u32(key_idx).as_raw(),
                          argc, result);
    }

    String key_str;
    if (!key_val.to_string(key_str))
        return false;

    guard.release();
    return call_keyed(obj_val, EsPropertyKey::from_str(key_str).as_raw(),
                      argc, result);
}

bool op_call_keyed(const EsValue &obj_val, uint64_t raw_key, int argc,
                   EsValue &result)
{
    return call_keyed(obj_val, raw_key, argc, result);
}

bool op_call_named(uint64_t raw_key, int argc,
                   EsValue &result)
{
    EsCallStackGuard guard(argc);

    EsPropertyKey key = EsPropertyKey::from_raw(raw_key);

    EsContext *ctx = EsContextStack::instance().top();

    EsValue this_value = es_get_this_value(ctx->lex_env(), key);
    if (this_value.is_nothing())
    {
        ES_THROW(EsReferenceError,
                 es_fmt_msg(ES_MSG_REF_NOT_DEFINED,
                            key.to_string().utf8().c_str()));
        return false;
    }

    EsValue fun;
    if (!op_ctx_get(ctx, key.as_raw(), fun, 0))
        return false;

    if (!fun.is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }

    int flags = 0;
    if (key == property_keys.eval)
        flags |= EsFunction::CALL_DIRECT_EVAL;

    guard.release();

    EsCallFrame frame = EsCallFrame::push_function_excl_args(
        argc, fun.as_function(), this_value);
    if (!fun.as_function()->callT(frame, flags))
        return false;

    result = frame.result();
    return true;
}

bool op_call_new(const EsValue &fun, int argc, EsValue &result)
{
    EsCallStackGuard guard(argc);

    // FIXME: Do we need this check?
    if (!fun.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_OBJ));
        return false;
    }

    if (!fun.is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }

    guard.release();

    EsCallFrame frame = EsCallFrame::push_function_excl_args(
        argc, fun.as_function(), EsValue::null);
    if (!fun.as_function()->constructT(frame))
        return false;

    result = frame.result();
    return true;
}

EsValue op_new_arr(int count, EsValue items[])
{
    return EsValue::from_obj(EsArray::create_inst_from_lit(count, items));
}

EsValue op_new_obj()
{
    return EsValue::from_obj(EsObject::create_inst());
}

EsValue op_new_fun_decl(EsContext *ctx, ES_API_FUN_PTR(fun),
                        bool strict, int prmc)
{
    assert(fun);

    EsFunction *obj = EsFunction::create_inst(ctx->var_env(), fun, strict, prmc);
    if (!obj)
        THROW(MemoryException);

    return EsValue::from_obj(obj);
}

EsValue op_new_fun_expr(EsContext *ctx, ES_API_FUN_PTR(fun),
                        bool strict, int prmc)
{
    assert(fun);

    EsFunction *obj = EsFunction::create_inst(ctx->lex_env(), fun,
                                              strict, prmc);
    if (!obj)
        THROW(MemoryException);

    return EsValue::from_obj(obj);
}

EsValue op_new_reg_exp(const String &pattern, const String &flags)
{
    return EsValue::from_obj(EsRegExp::create_inst(pattern, flags));
}

bool op_u_typeof(const EsValue &val, EsValue &result)
{
    switch (val.type())
    {
        case EsValue::TYPE_UNDEFINED:
            result = EsValue::from_str(_USTR("undefined"));
            return true;
        case EsValue::TYPE_NULL:
            result = EsValue::from_str(_USTR("object"));
            return true;
        case EsValue::TYPE_BOOLEAN:
            result = EsValue::from_str(_USTR("boolean"));
            return true;
        case EsValue::TYPE_NUMBER:
            result = EsValue::from_str(_USTR("number"));
            return true;
        case EsValue::TYPE_STRING:
            result = EsValue::from_str(_USTR("string"));
            return true;
        case EsValue::TYPE_OBJECT:
            result = EsValue::from_str(val.is_callable() ? _USTR("function") : _USTR("object"));
            return true;
        default:
            assert(false);
            break;
    }

    assert(false);
    result = EsValue::from_str(_USTR("undefined"));
    return true;
}

bool op_u_not(const EsValue &expr, EsValue &result)
{
    result = EsValue::from_bool(!expr.to_boolean());
    return true;
}

bool op_u_bit_not(const EsValue &expr, EsValue &result)
{
    int32_t old_value = 0;
    if (!expr.to_int32(old_value))
        return false;

    result = EsValue::from_i32(~old_value);
    return true;
}

bool op_u_add(const EsValue &expr, EsValue &result)
{
    double num = 0.0;
    if (!expr.to_number(num))
        return false;

    result = EsValue::from_num(num);
    return true;
}

bool op_u_sub(const EsValue &expr, EsValue &result)
{
    double old_num = 0.0;
    if (!expr.to_number(old_num))
        return false;

    if (old_num != old_num) // According to IEEE this is only true for NaN.
        result = EsValue::from_num(old_num);
    else
        result = EsValue::from_num(-old_num);

    return true;
}

bool op_b_or(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    int32_t lnum = 0;
    if (!lval.to_int32(lnum))
        return false;

    int32_t rnum = 0;
    if (!rval.to_int32(rnum))
        return false;
    
    result = EsValue::from_i32(lnum | rnum);
    return true;
}

bool op_b_xor(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    int32_t lnum = 0;
    if (!lval.to_int32(lnum))
        return false;

    int32_t rnum = 0;
    if (!rval.to_int32(rnum))
        return false;
    
    result = EsValue::from_i32(lnum ^ rnum);
    return true;
}

bool op_b_and(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    int32_t lnum = 0;
    if (!lval.to_int32(lnum))
        return false;

    int32_t rnum = 0;
    if (!rval.to_int32(rnum))
        return false;
    
    result = EsValue::from_i32(lnum & rnum);
    return true;
}

bool op_b_shl(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    int32_t lnum = 0;
    if (!lval.to_int32(lnum))
        return false;

    uint32_t rnum = 0;
    if (!rval.to_uint32(rnum))
        return false;
    
    result = EsValue::from_i32(lnum << (rnum & 0x1f));
    return true;
}

bool op_b_sar(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    int32_t lnum = 0;
    if (!lval.to_int32(lnum))
        return false;
    
    uint32_t rnum = 0;
    if (!rval.to_uint32(rnum))
        return false;

    result = EsValue::from_i32(lnum >> (rnum & 0x1f));
    return true;
}

bool op_b_shr(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    uint32_t lnum = 0;
    if (!lval.to_uint32(lnum))
        return false;

    uint32_t rnum = 0;
    if (!rval.to_uint32(rnum))
        return false;
    
    result = EsValue::from_u32(lnum >> (rnum & 0x1f));
    return true;
}

bool op_b_add(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    EsValue lprim;
    if (!lval.to_primitive(ES_HINT_NONE, lprim))
        return false;

    EsValue rprim;
    if (!rval.to_primitive(ES_HINT_NONE, rprim))
        return false;
    
    // If either of the objects is a string, the resulting type will be
    // string, otherwise the resulting type will be number.    
    if (lprim.is_string() || rprim.is_string())
    {
        result = EsValue::from_str(lprim.primitive_to_string() +
                                   rprim.primitive_to_string());
    }
    else
    {
        result = EsValue::from_num(lprim.primitive_to_number() +
                                   rprim.primitive_to_number());
    }

    return true;
}

bool op_b_sub(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    double lnum = 0.0;
    if (!lval.to_number(lnum))
        return false;
    double rnum = 0.0;
    if (!rval.to_number(rnum))
        return false;
    
    // Subtraction is always performed as numbers.
    result = EsValue::from_num(lnum - rnum);
    return true;
    
    // FIXME: 11.6.3
}

bool op_b_mul(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    double lnum = 0.0;
    if (!lval.to_number(lnum))
        return false;
    double rnum = 0.0;
    if (!rval.to_number(rnum))
        return false;
    
    // Multiplication is always performed as numbers.
    result = EsValue::from_num(lnum * rnum);
    return true;
}

bool op_b_div(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    double lnum = 0.0;
    if (!lval.to_number(lnum))
        return false;
    double rnum = 0.0;
    if (!rval.to_number(rnum))
        return false;

    // Division is always performed as numbers.
    result = EsValue::from_num(lnum / rnum);
    return true;
}

bool op_b_mod(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    double lnum = 0.0;
    if (!lval.to_number(lnum))
        return false;
    double rnum = 0.0;
    if (!rval.to_number(rnum))
        return false;
    
    // Modulo is always performed as numbers.
    result = EsValue::from_num(fmod(lnum, rnum));
    return true;
}

bool op_c_in(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    if (!rval.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_OBJ));
        return false;
    }
    
    String lstr;
    if (!lval.to_string(lstr))
        return false;

    result = EsValue::from_bool(rval.as_object()->has_property(EsPropertyKey::from_str(lstr)));
    return true;
}

bool op_c_instance_of(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    if (!rval.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_OBJ));
        return false;
    }
    
    if (!rval.is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    bool has_instance = false;
    if (!rval.as_function()->has_instanceT(lval, has_instance))
        return false;

    result = EsValue::from_bool(has_instance);
    return true;
}

bool op_c_strict_eq(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    result = EsValue::from_bool(algorithm::strict_eq_comp(lval, rval));
    return true;
}

bool op_c_strict_neq(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    result = EsValue::from_bool(!algorithm::strict_eq_comp(lval, rval));
    return true;
}

bool op_c_eq(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    bool test = false;
    if (!algorithm::abstr_eq_comp(lval, rval, test))
        return false;

    result = EsValue::from_bool(test);
    return true;
}

bool op_c_neq(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    bool test = false;;
    if (!algorithm::abstr_eq_comp(lval, rval, test))
        return false;

    result = EsValue::from_bool(!test);
    return true;
}

bool op_c_lt(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    Maybe<bool> test;
    if (!algorithm::abstr_rel_comp(lval, rval, true, test))
        return false;

    result = EsValue::from_bool(test && *test);
    return true;
}

bool op_c_gt(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    Maybe<bool> test;
    if (!algorithm::abstr_rel_comp(rval, lval, false, test))
        return false;

    result = EsValue::from_bool(test && *test);
    return true;
}

bool op_c_lte(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    Maybe<bool> test;
    if (!algorithm::abstr_rel_comp(rval, lval, false, test))
        return false;

    result = EsValue::from_bool(test && !*test);
    return true;
}

bool op_c_gte(const EsValue &lval, const EsValue &rval, EsValue &result)
{
    Maybe<bool> test;
    if (!algorithm::abstr_rel_comp(lval, rval, true, test))
        return false;

    result = EsValue::from_bool(test && !*test);
    return true;
}
