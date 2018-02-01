/**
 * @file   tiledb_cpp_api_config_iter.h
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
 * This file declares the C++ API for the TileDB Config Iter object.
 */

#ifndef TILEDB_TILEDB_CPP_API_CONFIG_ITER_H
#define TILEDB_TILEDB_CPP_API_CONFIG_ITER_H

#include "tiledb.h"
#include "tiledb_cpp_api_exception.h"
#include "tiledb_cpp_api_utils.h"

#include <iterator>
#include <functional>
#include <memory>

namespace tdb {

  class Config;

  namespace impl {

    class ConfigIter : public std::iterator<std::forward_iterator_tag, std::string> {
    public:

      /* ********************************* */
      /*     CONSTRUCTORS & DESTRUCTORS    */
      /* ********************************* */

      /** Iterate over a config for params matching a given prefix. **/
      ConfigIter(const Config &config,
                 const std::string &prefix,
                 bool done=false)
      : prefix_(prefix), done_(done) {
        init(config);
      }

      ConfigIter(const ConfigIter&) = delete;
      ConfigIter(ConfigIter&&) = default;
      ConfigIter &operator=(const ConfigIter&) = delete;
      ConfigIter &operator=(ConfigIter&&) = default;

      bool operator==(const ConfigIter &o) {
        return done_ == o.done_;
      }

      bool operator!=(const ConfigIter &o) {
        return done_ != o.done_;
      }

      const std::pair<std::string, std::string> &operator*() const {
        return here_;
      }

      const std::pair<std::string, std::string> *operator->() const {
        return &here_;
      }

      ConfigIter &operator++();

    private:
      /* ********************************* */
      /*          PRIVATE METHODS          */
      /* ********************************* */

      /** Init the iterator object **/
      void init(const Config &config);

      /** Prefix of parameters to match. **/
      const std::string &prefix_;

      /** Pointer to iter object. **/
      std::shared_ptr<tiledb_config_iter_t> iter_;

      /** Current object. **/
      std::pair<std::string, std::string> here_;

      /** If iter is done. **/
      bool done_;
    };

  }
}

#endif //TILEDB_TILEDB_CPP_API_CONFIG_ITER_H
