open Core.Std

type t =
  (* equivalent to [(string * int) list] *)
  (int) String.Map.t

let empty = String.Map.empty

let touch t str =
  let count =
    match String.Map.find t str with
    | None -> 0
    | Some x -> x
  in
  String.Map.add t str (count + 1)

let sorted t =
  List.sort ~cmp:(fun (k1, v1) (k2, v2) -> if v1 = v2
    then String.descending k1 k2
    else Int.descending v1 v2) (String.Map.to_alist t)

let top_n t n =
  List.take (sorted t) n
