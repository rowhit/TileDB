/**
 * @file  tdbpp_arrayschema.cc
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
 * This file declares the C++ API for TileDB.
 */

#include "tdbpp_arrayschema.h"
#include "tdbpp_attribute.h"
#include "tdbpp_context.h"
#include "tdbpp_domain.h"

#include <array>
#include <sstream>

void tdb::ArraySchema::_init(tiledb_array_schema_t *schema) {
  _schema = std::shared_ptr<tiledb_array_schema_t>(schema, _deleter);
}

void tdb::ArraySchema::_init(const std::string &uri) {
  tiledb_array_schema_t *schema;
  _ctx.get().handle_error(
      tiledb_array_schema_load(_ctx.get(), &schema, uri.c_str()));
  _init(schema);
}

void tdb::ArraySchema::dump(FILE *out) const {
  auto &ctx = _ctx.get();
  ctx.handle_error(tiledb_array_schema_dump(ctx, _schema.get(), out));
}

tiledb_array_type_t tdb::ArraySchema::type() const {
  auto &ctx = _ctx.get();
  tiledb_array_type_t type;
  ctx.handle_error(
      tiledb_array_schema_get_array_type(ctx, _schema.get(), &type));
  return type;
}

tdb::Compressor tdb::ArraySchema::coord_compressor() const {
  auto &ctx = _ctx.get();
  Compressor cmp;
  ctx.handle_error(tiledb_array_schema_get_coords_compressor(
      ctx, _schema.get(), &cmp.compressor, &cmp.level));
  return cmp;
}

tdb::ArraySchema &tdb::ArraySchema::set_coord_compressor(
    const tdb::Compressor c) {
  auto &ctx = _ctx.get();
  ctx.handle_error(tiledb_array_schema_set_coords_compressor(
      ctx, _schema.get(), c.compressor, c.level));
  return *this;
}

tdb::Compressor tdb::ArraySchema::offset_compressor() const {
  auto &ctx = _ctx.get();
  Compressor c;
  ctx.handle_error(tiledb_array_schema_get_offsets_compressor(
      ctx, _schema.get(), &c.compressor, &c.level));
  return c;
}

tdb::ArraySchema &tdb::ArraySchema::set_offset_compressor(
    const tdb::Compressor c) {
  auto &ctx = _ctx.get();
  ctx.handle_error(tiledb_array_schema_set_offsets_compressor(
      ctx, _schema.get(), c.compressor, c.level));
  return *this;
}

tdb::Domain tdb::ArraySchema::domain() const {
  auto &ctx = _ctx.get();
  tiledb_domain_t *domain;
  ctx.handle_error(tiledb_array_schema_get_domain(ctx, _schema.get(), &domain));
  return Domain(ctx, &domain);
}

tdb::ArraySchema &tdb::ArraySchema::set_domain(const Domain &domain) {
  auto &ctx = _ctx.get();
  ctx.handle_error(
      tiledb_array_schema_set_domain(ctx, _schema.get(), domain.ptr().get()));
  return *this;
}
tdb::ArraySchema &tdb::ArraySchema::add_attribute(const tdb::Attribute &attr) {
  auto &ctx = _ctx.get();
  ctx.handle_error(
      tiledb_array_schema_add_attribute(ctx, _schema.get(), attr.ptr().get()));
  return *this;
}

void tdb::ArraySchema::check() const {
  auto &ctx = _ctx.get();
  ctx.handle_error(tiledb_array_schema_check(ctx, _schema.get()));
}

const std::unordered_map<std::string, tdb::Attribute>
tdb::ArraySchema::attributes() const {
  auto &ctx = _ctx.get();
  tiledb_attribute_t *attrptr;
  unsigned int nattr;
  std::unordered_map<std::string, Attribute> attrs;
  ctx.handle_error(
      tiledb_array_schema_get_num_attributes(ctx, _schema.get(), &nattr));
  for (unsigned int i = 0; i < nattr; ++i) {
    ctx.handle_error(
        tiledb_attribute_from_index(ctx, _schema.get(), i, &attrptr));
    auto attr = Attribute(_ctx, &attrptr);
    attrs.emplace(
        std::pair<std::string, Attribute>(attr.name(), std::move(attr)));
  }
  return attrs;
}

tdb::ArraySchema &tdb::ArraySchema::create() {
  auto &ctx = _ctx.get();
  tiledb_array_schema_t *schema;
  ctx.handle_error(tiledb_array_schema_create(ctx, &schema));
  _schema = std::shared_ptr<tiledb_array_schema_t>(schema, _deleter);
  return *this;
}

tdb::ArraySchema &tdb::ArraySchema::set_cell_order(tiledb_layout_t layout) {
  auto &ctx = _ctx.get();
  ctx.handle_error(
      tiledb_array_schema_set_cell_order(ctx, _schema.get(), layout));
  return *this;
}

tdb::ArraySchema &tdb::ArraySchema::set_tile_order(tiledb_layout_t layout) {
  auto &ctx = _ctx.get();
  ctx.handle_error(
      tiledb_array_schema_set_tile_order(ctx, _schema.get(), layout));
  return *this;
}

tdb::ArraySchema &tdb::ArraySchema::set_order(
    const std::array<tiledb_layout_t, 2> &p) {
  set_tile_order(p[0]);
  set_cell_order(p[1]);
  return *this;
}

tiledb_layout_t tdb::ArraySchema::cell_order() const {
  auto &ctx = _ctx.get();
  tiledb_layout_t layout;
  ctx.handle_error(
      tiledb_array_schema_get_cell_order(ctx, _schema.get(), &layout));
  return layout;
}

tiledb_layout_t tdb::ArraySchema::tile_order() const {
  auto &ctx = _ctx.get();
  tiledb_layout_t layout;
  ctx.handle_error(
      tiledb_array_schema_get_tile_order(ctx, _schema.get(), &layout));
  return layout;
}

uint64_t tdb::ArraySchema::capacity() const {
  auto &ctx = _ctx.get();
  uint64_t capacity;
  ctx.handle_error(
      tiledb_array_schema_get_capacity(ctx, _schema.get(), &capacity));
  return capacity;
}

tdb::ArraySchema &tdb::ArraySchema::set_capacity(uint64_t capacity) {
  auto &ctx = _ctx.get();
  ctx.handle_error(
      tiledb_array_schema_set_capacity(ctx, _schema.get(), capacity));
  return *this;
}

tdb::ArraySchema &tdb::ArraySchema::set_kv() {
  auto &ctx = _ctx.get();
  ctx.handle_error(tiledb_array_schema_set_as_kv(ctx, _schema.get()));
  return *this;
}

bool tdb::ArraySchema::is_kv() const {
  auto &ctx = _ctx.get();
  int kv;
  ctx.handle_error(tiledb_array_schema_get_as_kv(ctx, _schema.get(), &kv));
  return kv != 0;
}

std::shared_ptr<tiledb_array_schema_t> tdb::ArraySchema::ptr() const {
  return _schema;
}

bool tdb::ArraySchema::good() const {
  return _schema == nullptr;
}

std::reference_wrapper<tdb::Context> tdb::ArraySchema::context() {
  return _ctx;
}

std::ostream &tdb::operator<<(std::ostream &os, const ArraySchema &schema) {
  os << "ArraySchema<";
  os << from_tiledb(schema.type());
  os << ' ' << schema.domain();
  for (const auto &a : schema.attributes()) {
    os << ' ' << a.second;
  }
  os << '>';
  return os;
}

tdb::ArraySchema &tdb::operator<<(ArraySchema &schema, const Domain &d) {
  schema.set_domain(d);
  return schema;
}

tdb::ArraySchema &tdb::operator<<(
    ArraySchema &schema, const tdb::Attribute &a) {
  schema.add_attribute(a);
  return schema;
}

tdb::ArraySchema &tdb::operator<<(
    ArraySchema &schema, const tiledb_array_type_t type) {
  schema.set_type(type);
  return schema;
}

tdb::ArraySchema &tdb::operator<<(
    ArraySchema &schema, const std::array<tiledb_layout_t, 2> p) {
  schema.set_order(p);
  return schema;
}

tdb::ArraySchema &tdb::operator<<(ArraySchema &schema, uint64_t capacity) {
  schema.set_capacity(capacity);
  return schema;
}