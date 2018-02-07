/**
 * @file   tiledb_cpp_api_query.h
 *
 * @author Ravi Gaddipati
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2018 TileDB, Inc.
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
 * This file declares the C++ API for the TileDB Query object.
 */

#ifndef TILEDB_CPP_API_QUERY_H
#define TILEDB_CPP_API_QUERY_H

#include "tiledb.h"
#include "tiledb_cpp_api_array_schema.h"
#include "tiledb_cpp_api_context.h"
#include "tiledb_cpp_api_core_interface.h"
#include "tiledb_cpp_api_deleter.h"
#include "tiledb_cpp_api_exception.h"
#include "tiledb_cpp_api_type.h"
#include "tiledb_cpp_api_utils.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace tdb {

/**
 * Construct and execute read/write queries on a tdb::Array
 */
class Query {
 public:
  /* ********************************* */
  /*           TYPE DEFINITIONS        */
  /* ********************************* */

  /** The query or query attribute status. */
  enum class Status { FAILED, COMPLETE, INPROGRESS, INCOMPLETE, UNDEF };

  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  Query(
      const Context &ctx,
      const std::string &array_uri,
      tiledb_query_type_t type);
  Query(const Query &) = default;
  Query(Query &&o) = default;
  Query &operator=(const Query &) = default;
  Query &operator=(Query &&o) = default;

  /* ********************************* */
  /*                API                */
  /* ********************************* */

  /** Sets the data layout of the buffers.  */
  Query &set_layout(tiledb_layout_t layout);

  /** Returns the query status. */
  Status query_status() const;

  /** Returns the query status for a particular attribute. */
  Status attribute_status(const std::string &attr) const;

  /** Submits the query. Call will block until query is complete. */
  Status submit();

  /** Submit an async query (non-blocking). */
  void submit_async();

  /**
   * Submit an async query, with callback.
   *
   * @param callback Callback function.
   * @param data Data to pass to callback.
   * @return Status of submitted query.
   */
  template <typename Fn>
  void submit_async(const Fn &callback) {
    std::function<void(void *)> wrapper = [&](void *) { callback(); };
    auto &ctx = ctx_.get();
    prepare_submission();
    ctx.handle_error(tdb::impl::tiledb_query_submit_async(
        ctx, query_.get(), wrapper, nullptr));
  }

  /**
   * Returns the number of elements in the result buffers. This is a map
   * from the attribute name to a pair of values. For a fixed-sized
   * attribute, the first value in the pair is the number of result
   * buffer elements, whereas the second is 0 (ignored). For variable-sized
   * attributes, the first value in the pair is the number of elements
   * in the offsets buffer, and the second value is the number of elements
   * in the data buffer.
   *
   */
  std::unordered_map<std::string, std::pair<uint64_t, uint64_t>>
  result_buffer_elements() const;

  /** Clears all attribute buffers. */
  void reset_buffers();

  /**
   * Sets a subarray, defined in the order dimensions were added.
   * Coordinates are inclusive.
   *
   * @tparam T A native data type.
   * @param pairs The subarray defined as pairs of [start, stop] per dimension.
   */
  template <typename T = uint64_t>
  typename std::enable_if<std::is_fundamental<T>::value, void>::type
  set_subarray(const std::vector<T> &pairs) {
    auto &ctx = ctx_.get();
    impl::type_check<typename impl::type_from_native<T>::type>(
        schema_.domain().type());
    if (pairs.size() != schema_.domain().dim_num() * 2) {
      throw SchemaMismatch(
          "Subarray should have num_dims * 2 values: (low, high) for each "
          "dimension.");
    }
    ctx.handle_error(
        tiledb_query_set_subarray(ctx, query_.get(), pairs.data()));

    subarray_cell_num_ = pairs[1] - pairs[0] + 1;
    for (unsigned i = 2; i < pairs.size() - 1; i += 2) {
      subarray_cell_num_ *= (pairs[i + 1] - pairs[i] + 1);
    }
  }

  /**
   * Sets a subarray, defined in the order dimensions were added.
   * Coordinates are inclusive.
   *
   * @tparam T A native data type.
   * @param pairs The subarray defined as pairs of [start, stop] per dimension.
   */
  template <typename T = uint64_t>
  typename std::enable_if<std::is_fundamental<T>::value, void>::type
  set_subarray(const std::vector<std::array<T, 2>> &pairs) {
    std::vector<T> buf;
    buf.reserve(pairs.size() * 2);
    std::for_each(
        pairs.begin(), pairs.end(), [&buf](const std::array<T, 2> &p) {
          buf.push_back(p[0]);
          buf.push_back(p[1]);
        });
    set_subarray(buf);
  }

  /** Sets a buffer for a fixed-sized attribute. */
  template <typename T>
  typename std::
      enable_if<std::is_fundamental<typename T::value_type>::value, void>::type
      set_buffer(const std::string &attr, T &buf) {
    set_buffer<typename impl::type_from_native<typename T::value_type>::type>(
        attr, buf);
  }

  /** Sets a buffer for a variable-sized attribute. */
  template <typename T>
  typename std::enable_if<
      std::is_fundamental<typename T::value_type>::value,
      void>::type
  set_buffer(const std::string &attr, std::vector<uint64_t> &offsets, T &buf) {
    set_buffer<typename impl::type_from_native<typename T::value_type>::type>(
        attr, offsets, buf);
  }

  /** Sets a buffer for a variable-sized attribute. */
  template <typename T>
  typename std::
      enable_if<std::is_fundamental<typename T::value_type>::value, void>::type
      set_buffer(
          const std::string &attr, std::pair<std::vector<uint64_t>, T> &buf) {
    set_buffer<typename impl::type_from_native<typename T::value_type>::type>(
        attr, buf.first, buf.second);
  }

  /* ********************************* */
  /*         STATIC FUNCTIONS          */
  /* ********************************* */

  /** Converts the TileDB C query status to a C++ query status. */
  static Status to_status(const tiledb_query_status_t &status);

  /** Converts the TileDB C query type to a string representation. */
  static std::string to_str(tiledb_query_type_t type);

 private:
  /* ********************************* */
  /*         PRIVATE ATTRIBUTES        */
  /* ********************************* */

  /** The buffers that will be passed to a TileDB C query. */
  std::vector<void *> all_buff_;

  /** On init get the attributes of the underlying array schema. */
  std::unordered_map<std::string, Attribute> array_attributes_;

  /** Attribute names for buffers set by the user for this query. */
  std::vector<std::string> attrs_;

  /** The attribute names that will be passed to a TileDB C query. */
  std::vector<const char *> attr_names_;

  /** The buffer sizes that will be passed to a TileDB C query. */
  std::vector<uint64_t> buff_sizes_;

  /** The TileDB context. */
  std::reference_wrapper<const Context> ctx_;

  /** Deleter wrapper. */
  impl::Deleter deleter_;

  /** Pointer to the TileDB C query object. */
  std::shared_ptr<tiledb_query_t> query_;

  /** The schema of the array the query targets at. */
  ArraySchema schema_;

  /** Number of cells set by `set_subarray`, influences `resize_buffer`. */
  uint64_t subarray_cell_num_ = 0;

  /**
   * Keeps track the offsets buffer of a variable-sized attribute.
   *
   * Format:
   * Size of the vector, size of vector::value_type, vector.data()
   */
  std::unordered_map<std::string, std::tuple<uint64_t, uint64_t, void *>>
      var_offsets_;

  /**
   * Keeps track the data buffer for an attribute.
   *
   * Format:
   * Size of the vector, size of vector::value_type, vector.data()
   */
  std::unordered_map<std::string, std::tuple<uint64_t, uint64_t, void *>>
      attr_buffs_;

  /** Keeps track of vector value_type sizes to convert back at return. */
  std::vector<uint64_t> sub_tsize_;

  /** URI of array being queried. **/
  std::string uri_;

  /* ********************************* */
  /*          PRIVATE METHODS          */
  /* ********************************* */

  /** Collate buffers and attach them to the query. */
  void prepare_submission();

  /**
   * Sets a buffer for a fixed-sized attribute.
   *
   * @tparam T Attribute/buffer type, tdb::impl::*
   * @param attr Attribute name
   * @param buf Data buffer
   * @return *this
   */
  template <typename T, typename Vec>
  typename std::enable_if<
      std::is_same<typename Vec::value_type, typename T::type>::value,
      Query>::type &
  set_buffer(const std::string &attr, Vec &buf) {
    if (array_attributes_.count(attr)) {
      const auto &a = array_attributes_.at(attr);
      impl::type_check_attr<T>(a, a.cell_val_num());
    } else if (attr == TILEDB_COORDS) {
      impl::type_check<T>(schema_.domain().type());
    } else {
      throw AttributeError("Attribute does not exist: " + attr);
    }
    attr_buffs_[attr] = std::make_tuple<uint64_t, uint64_t, void *>(
        static_cast<uint64_t>(buf.size()),
        sizeof(typename T::type),
        const_cast<void *>(reinterpret_cast<const void *>(
            buf.data())));  // To enable const char * storage
    attrs_.emplace_back(attr);
    return *this;
  }

  /**
   * Set a buffer for a variable-sized attribute.
   *
   * @tparam T buffer type, tdb::impl::*
   * @param attr Attribute name
   * @param offsets list of offsets for the data buffer
   * @param data Data buffer
   * @return *this
   */
  template <typename T, typename Vec>
  typename std::enable_if<
      std::is_same<typename Vec::value_type, typename T::type>::value,
      Query>::type &
  set_buffer(
      const std::string &attr, std::vector<uint64_t> &offsets, Vec &data) {
    if (array_attributes_.count(attr)) {
      impl::type_check_attr<T>(array_attributes_.at(attr), TILEDB_VAR_NUM);
    } else {
      throw AttributeError("Attribute does not exist: " + attr);
    }
    var_offsets_[attr] = std::make_tuple<uint64_t, uint64_t, void *>(
        offsets.size(), sizeof(uint64_t), offsets.data());
    attr_buffs_[attr] = std::make_tuple<uint64_t, uint64_t, void *>(
        static_cast<uint64_t>(data.size()),
        sizeof(typename T::type),
        const_cast<void *>(reinterpret_cast<const void *>(
            data.data())));  // To enable const char * storage
    attrs_.emplace_back(attr);
    return *this;
  }
};

/* ********************************* */
/*               MISC                */
/* ********************************* */

/** Get a string representation of a query status for an output stream. */
std::ostream &operator<<(std::ostream &os, const Query::Status &stat);

}  // namespace tdb

#endif  // TILEDB_CPP_API_QUERY_H
