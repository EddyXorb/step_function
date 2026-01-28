# About step_function

This data structure represents a step function from K to V.

## Complexity
It is O(log N) for both assignment and look-up, and uses as few O(log N)
calls as possible during assignment. It is near-to optimal or optimal.

A step function is a piecewise constant function, i.e., it is constant
on intervals. Therefore we can think about a stepfunction as a mapping from
intervals to values.
All intervals in this step function are
of the form (a,b], i.e., left-open, right-closed.

## Example 
After creation with start value "A":
(-infinity, +infinity) -> "A"

After assign(2,5,"B"):

(-infinity,2] -> "A"
(2,5]         -> "B"
(5,+infinity) -> "A"

After assign(4,7,"C"):

(-infinity,2] -> "A"
(2,4]         -> "B"
(4,7]         -> "C"
