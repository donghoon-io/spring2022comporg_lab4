/**
 * ECE 430.322: Computer Organization 
 * Lab 4: Cache Simulation
 */

#include "cache.h"
#include <iostream>
#include <string>
#include <cmath>

//Added
#include <vector>
#include <tuple>

/**
 * This allocates an "assoc" number of cache entries per a set
 * @param assoc - number of cache entries in a set
 */
cache_set_c::cache_set_c(int assoc) {
  m_entry = new cache_entry_c[assoc];
  m_assoc = assoc;
}

// cache_set_c destructor
cache_set_c::~cache_set_c() {
  delete[] m_entry;
}

/**
 * This constructor initializes a cache structure based on the cache parameters.
 * @param name - cache name; use any name you want
 * @param num_sets - number of sets in a cache
 * @param assoc - number of cache entries in a set
 * @param line_size - cache block (line) size in bytes // 이게 B
 */
cache_c::cache_c(std::string name, int num_sets, int assoc, int line_size) {
  m_name = name;
  m_num_sets = num_sets;
  m_line_size = line_size;

  m_set = new cache_set_c *[m_num_sets];

  for (int ii = 0; ii < m_num_sets; ++ii) {
    m_set[ii] = new cache_set_c(assoc);

    // initialize tag/valid/dirty bits
    for (int jj = 0; jj < assoc; ++jj) {
      m_set[ii]->m_entry[jj].m_valid = false;
      m_set[ii]->m_entry[jj].m_dirty = false;
      m_set[ii]->m_entry[jj].m_tag   = 0;
    }
  }

  // initialize stats
  m_num_accesses = 0;
  m_num_hits = 0;
  m_num_misses = 0;
  m_num_writes = 0;
  m_num_writebacks = 0;
}

// cache_c destructor
cache_c::~cache_c() {
  for (int ii = 0; ii < m_num_sets; ++ii) { delete m_set[ii]; }
  delete[] m_set;
}

/** 
 * This function looks up in the cache for a memory reference.
 * This needs to update all the necessary meta-data (e.g., tag/valid/dirty) 
 * and the cache statistics, depending on a cache hit or a miss.
 * @param address - memory address 
 * @param access_type - read (0), write (1), or instruction fetch (2)
 */
void cache_c::access(addr_t address, int access_type) {
  ////////////////////////////////////////////////////////////////////
  // TODO: Write the code to implement this function
  ////////////////////////////////////////////////////////////////////

  m_num_accesses += 1;

  int tag_from_address = address >> int(log2(m_num_sets * m_line_size));
  int idx_from_address = address >> int(log2(m_line_size)) & (m_num_sets-1);

  bool hit;
  int hit_idx;
  std::tie(hit, hit_idx) = check_if_hit(idx_from_address, tag_from_address);

  switch (access_type) {
  case 0: // read
    if ((tag_from_address == (m_set[idx_from_address]->m_entry[0].m_tag)) && (m_set[idx_from_address]->m_entry[0].m_valid)) {
      m_num_hits += 1;
      
      m_set[idx_from_address]->LRU_stack.erase(std::remove(m_set[idx_from_address]->LRU_stack.begin(), m_set[idx_from_address]->LRU_stack.end(), hit_idx), m_set[idx_from_address]->LRU_stack.end());
      m_set[idx_from_address]->LRU_stack.insert(m_set[idx_from_address]->LRU_stack.begin(), hit_idx);
    }
    else {
      m_num_misses += 1;

      if (!hit && (hit_idx != -1)) {
        m_set[idx_from_address]->LRU_stack.erase(std::remove(m_set[idx_from_address]->LRU_stack.begin(), m_set[idx_from_address]->LRU_stack.end(), hit_idx), m_set[idx_from_address]->LRU_stack.end());
        m_set[idx_from_address]->LRU_stack.insert(m_set[idx_from_address]->LRU_stack.begin(), hit_idx);
      }
      else {
        hit_idx = m_set[idx_from_address]->LRU_stack.back();
        m_num_writebacks += m_set[idx_from_address]->m_entry[hit_idx].m_dirty;
        
        std::rotate(m_set[idx_from_address]->LRU_stack.rbegin(), m_set[idx_from_address]->LRU_stack.rbegin() + 1, m_set[idx_from_address]->LRU_stack.rend());
      }

      m_set[idx_from_address]->m_entry[hit_idx].m_tag = tag_from_address;
      m_set[idx_from_address]->m_entry[hit_idx].m_dirty = false;
      m_set[idx_from_address]->m_entry[hit_idx].m_valid = true;
    }
    break;
  case 1: // write
    m_num_writes += 1;
    if ((tag_from_address == (m_set[idx_from_address]->m_entry[0].m_tag)) && (m_set[idx_from_address]->m_entry[0].m_valid)) {
      m_num_hits += 1;
      m_set[idx_from_address]->m_entry[hit_idx].m_dirty = true;

      m_set[idx_from_address]->LRU_stack.erase(std::remove(m_set[idx_from_address]->LRU_stack.begin(), m_set[idx_from_address]->LRU_stack.end(), hit_idx), m_set[idx_from_address]->LRU_stack.end());
      m_set[idx_from_address]->LRU_stack.insert(m_set[idx_from_address]->LRU_stack.begin(), hit_idx);
    }
    else {
      m_num_misses += 1;

      if (!hit && (hit_idx != -1)) {
        m_set[idx_from_address]->LRU_stack.erase(std::remove(m_set[idx_from_address]->LRU_stack.begin(), m_set[idx_from_address]->LRU_stack.end(), hit_idx), m_set[idx_from_address]->LRU_stack.end());
        m_set[idx_from_address]->LRU_stack.insert(m_set[idx_from_address]->LRU_stack.begin(), hit_idx);
      }
      else {
        hit_idx = m_set[idx_from_address]->LRU_stack.back();
        m_num_writebacks += m_set[idx_from_address]->m_entry[hit_idx].m_dirty;
        
        std::rotate(m_set[idx_from_address]->LRU_stack.rbegin(), m_set[idx_from_address]->LRU_stack.rbegin() + 1, m_set[idx_from_address]->LRU_stack.rend());
      }

      m_set[idx_from_address]->m_entry[hit_idx].m_tag = tag_from_address;
      m_set[idx_from_address]->m_entry[hit_idx].m_dirty = true;
      m_set[idx_from_address]->m_entry[hit_idx].m_valid = true;
    }
    break;
  case 2: // instruction fetch
    if ((tag_from_address == (m_set[idx_from_address]->m_entry[0].m_tag)) && (m_set[idx_from_address]->m_entry[0].m_valid)) {
      m_num_hits += 1;

      m_set[idx_from_address]->LRU_stack.erase(std::remove(m_set[idx_from_address]->LRU_stack.begin(), m_set[idx_from_address]->LRU_stack.end(), hit_idx), m_set[idx_from_address]->LRU_stack.end());
      m_set[idx_from_address]->LRU_stack.insert(m_set[idx_from_address]->LRU_stack.begin(), hit_idx);
    }
    else {
      m_num_misses += 1;

      if (!hit && (hit_idx != -1)) {
        m_set[idx_from_address]->LRU_stack.erase(std::remove(m_set[idx_from_address]->LRU_stack.begin(), m_set[idx_from_address]->LRU_stack.end(), hit_idx), m_set[idx_from_address]->LRU_stack.end());
        m_set[idx_from_address]->LRU_stack.insert(m_set[idx_from_address]->LRU_stack.begin(), hit_idx);
      }
      else {
        hit_idx = m_set[idx_from_address]->LRU_stack.back();
        m_num_writebacks += m_set[idx_from_address]->m_entry[hit_idx].m_dirty;

        std::rotate(m_set[idx_from_address]->LRU_stack.rbegin(), m_set[idx_from_address]->LRU_stack.rbegin() + 1, m_set[idx_from_address]->LRU_stack.rend());
      }

      m_set[idx_from_address]->m_entry[hit_idx].m_tag = tag_from_address;
      m_set[idx_from_address]->m_entry[hit_idx].m_dirty = false;
      m_set[idx_from_address]->m_entry[hit_idx].m_valid = true;
    }
    break;
  default:
    break;
  }
}

/**
 * Print statistics (DO NOT CHANGE)
 */
void cache_c::print_stats() {
  std::cout << "------------------------------" << "\n";
  std::cout << m_name << " Hit Rate: "          << (double)m_num_hits/m_num_accesses*100 << " % \n";
  std::cout << "------------------------------" << "\n";
  std::cout << "number of accesses: "    << m_num_accesses << "\n";
  std::cout << "number of hits: "        << m_num_hits << "\n";
  std::cout << "number of misses: "      << m_num_misses << "\n";
  std::cout << "number of writes: "      << m_num_writes << "\n";
  std::cout << "number of writebacks: "  << m_num_writebacks << "\n";
}

