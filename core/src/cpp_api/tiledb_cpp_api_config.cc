/**
 * @file   tiledb_cpp_api_config.cc
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
 * This file defines the C++ API for the TileDB Config object.
 */

#include "tiledb_cpp_api_config.h"

namespace tdb {

/* ********************************* */
/*     CONSTRUCTORS & DESTRUCTORS    */
/* ********************************* */

Config::Config() {
  create_config();
}

Config::Config(const std::string &filename) {
  create_config();
  tiledb_error_t *err;
  tiledb_config_load_from_file(config_.get(), filename.c_str(), &err);
  impl::check_error(err);
}

Config::Config(tiledb_config_t **config) {
  if (config) {
    config_ = std::shared_ptr<tiledb_config_t>(*config, tiledb_config_free);
    *config = nullptr;
  }
}

/* ********************************* */
/*                API                */
/* ********************************* */

std::shared_ptr<tiledb_config_t> Config::ptr() const {
  return config_;
}

Config::operator tiledb_config_t *() const {
  return config_.get();
}

Config &Config::set(const std::string &param, const std::string &value) {
  tiledb_error_t *err;
  tiledb_config_set(config_.get(), param.c_str(), value.c_str(), &err);
  impl::check_error(err);
  return *this;
}

std::string Config::get(const std::string &param) const {
  const char *val;
  tiledb_error_t *err;
  tiledb_config_get(config_.get(), param.c_str(), &val, &err);
  impl::check_error(err);

  return val;
}

impl::ConfigProxy Config::operator[](const std::string &param) {
  return {*this, param};
}

Config &Config::unset(const std::string &param) {
  tiledb_error_t *err;
  tiledb_config_unset(config_.get(), param.c_str(), &err);
  impl::check_error(err);

  return *this;
}

/* ********************************* */
/*          PRIVATE METHODS          */
/* ********************************* */

void Config::create_config() {
  tiledb_config_t *config;
  tiledb_error_t *err;
  tiledb_config_create(&config, &err);
  impl::check_error(err);

  config_ = std::shared_ptr<tiledb_config_t>(config, tiledb_config_free);
}

}  // namespace tdb
