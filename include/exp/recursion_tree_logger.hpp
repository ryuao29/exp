#pragma once

#include "exp/logger.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace mce {

inline constexpr std::size_t kNoParentNode = std::numeric_limits<std::size_t>::max();

struct RecursionTreeNodeRecord {
  std::size_t node_id = 0;
  std::size_t parent_id = kNoParentNode;
  std::size_t depth = 0;
  std::size_t clique_size = 0;
  std::string clique_vertices;
  std::size_t p_size = 0;
  std::size_t x_size = 0;
  std::size_t candidate_count = 0;
  std::size_t child_count = 0;
  std::uint64_t elapsed_us = 0;
  std::uint8_t is_leaf = 0;
};

class RecursionTreeCsvLogger {
public:
  RecursionTreeCsvLogger() = default;

  explicit RecursionTreeCsvLogger(const std::string& path, bool enabled = true, bool include_clique_vertices = false)
      : enabled_(enabled), include_clique_vertices_(include_clique_vertices), logger_(path, enabled) {
    if (enabled_) {
      if (include_clique_vertices_) {
        logger_.write_header({"node_id", "parent_id", "depth", "clique_size", "clique_vertices", "p_size",
                              "x_size", "candidate_count", "child_count", "elapsed_us", "is_leaf"});
      } else {
        logger_.write_header({"node_id", "parent_id", "depth", "clique_size", "p_size", "x_size",
                              "candidate_count", "child_count", "elapsed_us", "is_leaf"});
      }
    }
  }

  bool enabled() const { return enabled_; }

  std::size_t next_node_id() { return next_node_id_++; }

  void write_node(const RecursionTreeNodeRecord& record) {
    if (!enabled_) {
      return;
    }
    const std::string parent_id = record.parent_id == kNoParentNode ? "-1" : std::to_string(record.parent_id);
    if (include_clique_vertices_) {
      logger_.write_row({std::to_string(record.node_id),
                         parent_id,
                         std::to_string(record.depth),
                         std::to_string(record.clique_size),
                         record.clique_vertices,
                         std::to_string(record.p_size),
                         std::to_string(record.x_size),
                         std::to_string(record.candidate_count),
                         std::to_string(record.child_count),
                         std::to_string(record.elapsed_us),
                         std::to_string(static_cast<unsigned int>(record.is_leaf))});
    } else {
      logger_.write_row({std::to_string(record.node_id),
                         parent_id,
                         std::to_string(record.depth),
                         std::to_string(record.clique_size),
                         std::to_string(record.p_size),
                         std::to_string(record.x_size),
                         std::to_string(record.candidate_count),
                         std::to_string(record.child_count),
                         std::to_string(record.elapsed_us),
                         std::to_string(static_cast<unsigned int>(record.is_leaf))});
    }
  }

  void flush() {
    if (enabled_) {
      logger_.flush();
    }
  }

private:
  bool enabled_ = false;
  bool include_clique_vertices_ = false;
  std::size_t next_node_id_ = 0;
  CsvLogger logger_;
};

}  // namespace mce