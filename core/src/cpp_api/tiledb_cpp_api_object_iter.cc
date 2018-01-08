/**
 * @file   tiledb_cpp_api_object_iter.cc
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
 * This file defines the C++ API for the TileDB ObjectIter object.
 */

#include "tiledb_cpp_api_object_iter.h"

namespace tdb {

/* ********************************* */
/*     CONSTRUCTORS & DESTRUCTORS    */
/* ********************************* */

ObjectIter::ObjectIter(Context &ctx, const std::string &root)
    : ctx_(ctx)
    , root_(root) {
  recursive_ = false;
  walk_order_ = TILEDB_PREORDER;
  group_ = true;
  array_ = true;
  kv_ = true;
  retrieve_objs_ = true;
}

/* ********************************* */
/*                API                */
/* ********************************* */

void ObjectIter::set_iter_policy(bool group, bool array, bool kv) {
  if (group_ != group || array_ != array || kv_ != kv)
    retrieve_objs_ = true;

  group_ = group;
  array_ = array;
  kv_ = kv;
}

void ObjectIter::set_recursive(tiledb_walk_order_t walk_order) {
  if (!recursive_ || walk_order_ != walk_order)
    retrieve_objs_ = true;

  recursive_ = true;
  walk_order_ = walk_order;
}

/* ********************************* */
/*         STATIC FUNCTIONS          */
/* ********************************* */

int ObjectIter::obj_getter(const char *path, tiledb_object_t type, void *data) {
  auto data_struct = static_cast<ObjGetterData *>(data);
  if ((type == TILEDB_ARRAY && data_struct->array_) ||
      (type == TILEDB_GROUP && data_struct->group_) ||
      (type == TILEDB_KEY_VALUE && data_struct->kv_)) {
    Object obj(type, path);
    auto &objs = data_struct->objs_.get();
    objs.push_back(std::move(obj));
  }
  return 1;
}

ObjectIter::iterator ObjectIter::begin() {
  if (retrieve_objs_) {
    objs_.clear();
    auto &ctx = ctx_.get();
    ObjGetterData data(objs_, array_, group_, kv_);
    if (recursive_)
      ctx.handle_error(tiledb_object_walk(
          ctx.ptr(), root_.c_str(), walk_order_, obj_getter, &data));
    else
      ctx.handle_error(
          tiledb_object_ls(ctx.ptr(), root_.c_str(), obj_getter, &data));
    retrieve_objs_ = false;
  }

  return iterator(objs_);
}

ObjectIter::iterator ObjectIter::end() {
  return iterator(objs_).end();
}

}  // namespace tdb
