#include <limits>
#include <map>
#include <optional>

/// This data structure represents a step function from K to V.
///
/// Complexity: It is O(log N) for both assignment and look-up, and uses as few O(log N)
/// calls as possible during assignment. It is near-to optimal or optimal.
///
/// A step function is a piecewise constant function, i.e., it is constant
/// on intervals. Therefore we can think about a stepfunction as a mapping from
/// intervals to values.
/// All intervals in this step function are
/// of the form (a,b], i.e., left-open, right-closed.
///
/// Example:
/// After creation with start value "A":
/// (-infinity, +infinity) -> "A"
///
/// After assign(2,5,"B"):
///
/// (-infinity,2] -> "A"
/// (2,5]         -> "B"
/// (5,+infinity) -> "A"
///
/// After assign(4,7,"C"):
///
/// (-infinity,2] -> "A"
/// (2,4]         -> "B"
/// (4,7]         -> "C"
/// (7,+infinity) -> "A"
template <typename K, typename V>
class step_function
{
public:
   V start_value;
   std::map<K, V> steps;

   step_function(V const& val) : start_value(val) {}

   void assign(K const& keyBegin, K const& keyEnd, V const& val)
   {
      if (!(keyBegin < keyEnd))
         return;

      // naming: a = keyBegin, b = keyEnd, F_b = F(b) = (*this)[b] (for every b)
      // Interval set M can be decomposed into three sets: M = L u C u R, where
      // L := {(k,v) in M| k <a}, C:={(k,v) in M| a <= k < b}, R:= {(k,v) in M| b <= k}
      // The scope is to obtain a new set M' = L u C' u R' where
      // L remains unchanged but R and C are (possibly) changed.

      // C' contains at most two elements:
      // (a,val) and (b, F(b))
      // The following applys:
      // (a,val) in C' <=> left_lim(i->a)F(i) != val
      // and
      // (b,F(b)) in C' <=> F(b) != val and b < min(R)_k
      // where  min(R) meaning the entry (k,v) for k < k_x for every (k_x,v_x) in R with K_x != k
      // Strategy: we go backwards beginning at min(R)_k, adjust R and then adjust C, everything on the fly
      // and trying to reuse existing V's.

      if (steps.empty())
      {
         if (val == start_value)
            return;
         steps.emplace_hint(steps.end(), keyBegin, val);
         steps.emplace_hint(steps.end(), keyEnd, start_value);
         return;
      }
      // from here on the map is not empty
      assert(!steps.empty());

      auto it = steps.lower_bound(keyEnd);  // the only log N operation

      const V* F_a_left_limes = nullptr;  // means lim (i -> a) F(i)
      const V* F_b = nullptr;
      bool R_is_empty = it == steps.end();
      bool R_has_gap = !R_is_empty && keyEnd < it->first;
      std::optional<typename decltype(steps)::node_type> wrong_key_correct_val;

      // adjust R
      if (!R_is_empty && !(R_has_gap) && it->second == val)
      {
         F_b = &val;

         auto nextIt = std::next(it);
         wrong_key_correct_val = steps.extract(it);
         it = nextIt;

         R_has_gap = true;
         R_is_empty = it == steps.end();
      }

      bool should_not_insert_b_Fb = !R_is_empty && !R_has_gap;
      if (!should_not_insert_b_Fb)
      {
         bool F_b_equals_val = F_b != nullptr;
         if (!F_b)
         {
            // determine F(b), "it" is pointing to first entry of R, if R is not empty
            if (it != steps.begin())  // a value before R needs to be retrieved
            {
               F_b = &std::prev(it)->second;  // first in C
            }
            else  // it is either end or it->first > keyBegin
            {
               F_b = &start_value;
               F_a_left_limes = &start_value;  // before b there is nothing in map, because it == steps.begin(), which
                                               // is either steps.end() or greater than b
            }
            assert(F_b);

            F_b_equals_val = *F_b == val;
         }

         // determine if should insert (b,F(b))
         if (!F_b_equals_val)
         {
            if (it != steps.begin() && !(std::prev(it)->first < keyBegin))
            {
               // we know prev now contains F_b and can be moved onto end to save creation of V
               auto node = steps.extract(std::prev(it));
               node.key() = keyEnd;
               assert(node.mapped() == *F_b);
               it = steps.insert(it, std::move(node));
            }
            else
            {
               it = steps.emplace_hint(it, keyEnd, *F_b);  // we could cache the it and
            }
         }
      }

      // FINISHED (b,F(b)) treatment

      assert(it == steps.end() || !(it->first < keyEnd));

      auto lower_bound_a =
          it;  // from now on we erase all elements before it, therefore save the current position for later

      while (it != steps.begin() && !((--it)->first < keyBegin))
      {
         bool it_is_greater = keyBegin < it->first;

         if (!wrong_key_correct_val && it_is_greater && it->second == val)
         {
            auto nextIt = std::next(it);
            wrong_key_correct_val = steps.extract(it);
            it = nextIt;
         }
         else if (it_is_greater || !(it->second == val))  // do not delete correct value in correct position
         {
            it = steps.erase(it);
         }
         else  // !it_is_greater and value is equal, which is means, that (a,val) is already present
         {
            return;
         }
      }

      // FINISHED DELETION OF UNNEDED NODES IN C

      // determine lim(i -> a)F(i)
      if (!F_a_left_limes)
      {
         bool no_entry_left_from_keyBegin =
             it == steps.end() || keyBegin < it->first || (!(it->first < keyBegin) && it == steps.begin());

         if (no_entry_left_from_keyBegin)
         {
            F_a_left_limes = &start_value;
         }
         else if (it->first < keyBegin)  // !(keyBegin < it->first)
         {
            F_a_left_limes = &it->second;
         }
         else  // it->first is equivalent to keyBegin
         {
            F_a_left_limes = &std::prev(it)->second;
         }
      }

      assert(F_a_left_limes);

      bool should_add_a_fa = !(*F_a_left_limes == val);
      if (!should_add_a_fa)
         return;

      bool hint_points_to_keyBegin = lower_bound_a != steps.end() && !(keyBegin < lower_bound_a->first);
      if (wrong_key_correct_val)
      {
         auto& handle = wrong_key_correct_val.value();
         handle.key() = keyBegin;
         assert(handle.mapped() == val);
         if (hint_points_to_keyBegin)
         {
            lower_bound_a->second = std::move(handle.mapped());
         }
         else
         {
            steps.insert(lower_bound_a, std::move(handle));
         }
      }
      else
      {
         steps.insert_or_assign(lower_bound_a, keyBegin, val);
      }
   }

   // look-up of the value associated with key
   V const& operator[](K const& key) const
   {
      auto it = steps.upper_bound(key);
      if (it == steps.begin())
      {
         return start_value;
      }
      else
      {
         return (--it)->second;
      }
   }

   bool sanityCheck()
   {
      const auto* currentValue = &start_value;
      for (const auto& [k, v] : steps)
      {
         if (*currentValue == v)
            return false;
         currentValue = &v;
      }
      return true;
   }
};
