open Core.Std

(* Exercises:

   - Change the underlying representation to use [String.Map.t]

   - Add two functions

       val median_element : t -> string
       val median_occurrences : t -> int

   - Technically, if there are an even number of unique elements,
     there is no single median element, but rather one element before
     the median and one element after. Change the median function to
     reflect this.

   - Make the median functions O(1).
*)

type t

val empty : t
val touch : t -> string -> t
val top_n : t -> int -> (string * int) list
val sorted : t -> (string * int) list
