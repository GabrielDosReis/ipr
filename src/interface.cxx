//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis <gdr@cs.tamu.edu>
// 

#include "ipr/interface"

namespace ipr {

   namespace stats {
      static int node_total_count = 0;
      static int node_usage_counts[last_code_cat];

      int
      all_nodes_count()
      {
         return node_total_count;
      }

      int
      node_count(Category_code c)
      {
         // FIXME: check that "c" is in bounds.
         return node_usage_counts[c];
      }
      
   }

   Node::Node(Category_code c)
         : node_id(stats::node_total_count++), category(c)
   {
      // FIXME: Implement checking of "c".
      ++stats::node_usage_counts[c];
   }
};
