/**
 * @file  tiledb_cpp_api_map.cc
 *
 * @author Ravi Gaddipati
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the C++ API for the TileDB Map Item object.
 */

#ifndef TILEDB_TILEDB_CPP_API_MAP_ITEM_H
#define TILEDB_TILEDB_CPP_API_MAP_ITEM_H

#include "tiledb.h"
#include "tiledb_cpp_api_exception.h"
#include "tiledb_cpp_api_attribute.h"

#include <memory>
#include <functional>
#include <type_traits>
#include <tuple>
#include <utility>

namespace tdb {

/** Forward declarations. **/
class Map;
namespace impl {
  class MultiMapItemProxy;
  class MapItemProxy;
}

/** Object representing a Map key and its values. **/
class MapItem {
public:
  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  /** Load a MapItem given a pointer. **/
  MapItem(const Context &ctx, tiledb_kv_item_t **item, Map *map=nullptr)
  : ctx_(ctx), deleter_(ctx), map_(map) {
    item_ = std::shared_ptr<tiledb_kv_item_t>(*item, deleter_);
    *item = nullptr;
  }

  MapItem(const MapItem &) = default;
  MapItem(MapItem &&o) = default;
  MapItem &operator=(const MapItem &) = default;
  MapItem &operator=(MapItem &&o) = default;

  /* ********************************* */
  /*                API                */
  /* ********************************* */

  /** Set an attribute to the given value **/
  template<typename T>
  void set(const std::string &attr, const T &val) {
    set_impl(attr, val);
  }

  /**
   * Get the key for the item.
   *
   * @tparam T
   */
  template<typename T>
  T key() const {
    return key_impl<T>();
  }

  /** Get tiledb datatype and size **/
  std::pair<tiledb_datatype_t, uint64_t>key_type() const {
    auto &ctx = ctx_.get();
    const void* key;
    tiledb_datatype_t type;
    uint64_t size;
    ctx.handle_error(tiledb_kv_item_get_key(ctx, item_.get(), &key, &type, &size));
    return {type, size};
  }

  /**
   * Get the value for a given attribute.
   *
   * @tparam T Datatype of attr.
   * @param attr Attribute name
   * @param expected_num Expected number of elements, if not variable.
   * @return Pair of <T*, number of elements>
   */
  template <typename T>
  std::pair<const T*, uint64_t>
  get_ptr(const std::string &attr) const {
    const Context &ctx = ctx_.get();

    const T *data;
    tiledb_datatype_t type;
    uint64_t size;

    ctx.handle_error(tiledb_kv_item_get_value(
        ctx, item_.get(), attr.c_str(),(const void**)&data, &type, &size));

    impl::type_check<typename impl::type_from_native<T>::type>(type);

    unsigned num = static_cast<unsigned>(size/sizeof(T));
    return std::pair<const T*, uint64_t>(data, num);
  }

  /** Get a attribute with a given return type. **/
  template<typename T>
  T get(const std::string &attr) const {
    return get_impl<T>(attr);
  }

  /** Get a proxy to set/get with operator[] **/
  impl::MapItemProxy operator[](const std::string &attr);

  impl::MultiMapItemProxy operator[](const std::vector<std::string> &attrs);;

  /** Ptr to underlying object. **/
  std::shared_ptr<tiledb_kv_item_t> ptr() const {
    return item_;
  }

private:
  friend class Map;
  friend class impl::MapItemProxy;
  friend class impl::MultiMapItemProxy;

  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  /** Make an item with the given key. **/
  MapItem(const Context &ctx, void *key,
          tiledb_datatype_t type, size_t size, Map *map=nullptr)
  : ctx_(ctx), deleter_(ctx), map_(map) {
    tiledb_kv_item_t *p;
    ctx.handle_error(tiledb_kv_item_create(ctx, &p));
    ctx.handle_error(tiledb_kv_item_set_key(ctx, p, key, type, size));
    item_ = std::shared_ptr<tiledb_kv_item_t>(p, deleter_);
  }

  /* ********************************* */
  /*        TYPE SPECIALIZATIONS       */
  /* ********************************* */

  template<typename T, typename = typename T::value_type>
  T key_impl() const {
    auto &ctx = ctx_.get();
    T ret;
    const typename T::value_type* key;
    tiledb_datatype_t type;
    uint64_t size;
    ctx.handle_error(tiledb_kv_item_get_key(ctx, item_.get(),
                                            (const void **)&key, &type, &size));
    impl::type_check<typename impl::type_from_native<typename T::value_type>::type>(type);
    unsigned num = (unsigned)size / sizeof(typename T::value_type);
    ret.resize(num);
    std::copy(key, key + num, std::begin(ret));
    return ret;
  };

  template<typename T>
  typename std::enable_if<std::is_fundamental<T>::value, T>::type
  key_impl() const {
    auto &ctx = ctx_.get();
    const T* key;
    tiledb_datatype_t type;
    uint64_t size;
    ctx.handle_error(tiledb_kv_item_get_key(ctx, item_.get(),
                                            (const void**)&key, &type, &size));
    impl::type_check<typename impl::type_from_native<T>::type>(type);
    unsigned num = (unsigned)size / sizeof(T);
    if (num != 1) throw TileDBError("Expected key size of 1, got " + std::to_string(num));
    return *key;
  };

  /** Set an attribute to the given value, fundamental type. **/
  template <typename V>
  typename std::enable_if<std::is_fundamental<V>::value, void>::type
  set_impl(const std::string &attr, const V &val) {
    auto &ctx = ctx_.get();
    using AttrT = typename impl::type_from_native<V>::type;
    ctx.handle_error(tiledb_kv_item_set_value(ctx, item_.get(), attr.c_str(),
                                              &val, AttrT::tiledb_datatype, sizeof(V)));
  }

  /** Set an attribute to the given value, compound type. **/
  template <typename V>
  typename std::enable_if<std::is_fundamental<typename V::value_type>::value, void>::type
  set_impl(const std::string &attr, const V &val) {
    auto &ctx = ctx_.get();
    using AttrT = typename impl::type_from_native<typename V::value_type>::type;
    ctx.handle_error(
        tiledb_kv_item_set_value(ctx, item_.get(), attr.c_str(),
                                 const_cast<void*>(reinterpret_cast<const void*>(val.data())),
                                 AttrT::tiledb_datatype, sizeof(typename V::value_type) * val.size()));
  }

  void set_impl(const std::string &attr, const char *c) {
    set_impl(attr, std::string(c));
  }

  /**
 * Get a value as a compound type.
 *
 * @tparam T Data type.
 * @tparam V Container type, default vector.
 * @param attr Attribute name
 * @return value
 */
  template <typename T, typename = typename T::value_type>
  T get_impl(const std::string &attr) const {
    const typename T::value_type *data;
    unsigned num;
    std::tie(data, num) = get_ptr<typename T::value_type>(attr);

    T ret;
    ret.resize(num);
    std::copy(data, data + num,
              const_cast<typename T::value_type*>(ret.data()));
    return ret;
  }

  /** Get an attribute value as a fundamental type **/
  template <typename T>
  typename std::enable_if<std::is_fundamental<T>::value, T>::type
  get_impl(const std::string &attr) const {
    const T *data;
    unsigned num;
    std::tie(data, num) = get_ptr<T>(attr);
    if (num != 1) {
      throw AttributeError("Expected one element, got " + std::to_string(num));
    }
    return *data;
  }

  std::reference_wrapper<const Context> ctx_;

  /** Ptr to TileDB object. **/
  std::shared_ptr<tiledb_kv_item_t> item_;

  /** Deleter for tiledb object **/
  impl::Deleter deleter_;

  /** Underlying Map **/
  Map *map_ = nullptr;
};

}

#endif //TILEDB_TILEDB_CPP_API_MAP_ITEM_H
