open Core.Std

let build_counts counts =
  In_channel.fold_lines stdin ~init:counts ~f:(fun counts line ->
    Counter.touch counts line
  )

let count_lines () =
  let counter = build_counts Counter.empty in
  List.iter (Counter.top_n counter 10) ~f:(fun (line, count) ->
    printf "%7d %s\n" count line)

let command =
  Command.basic
    ~summary:"equivalent of \"| sort | uniq -c | sort -nr\""
    Command.Spec.(
      empty
    )
    (fun () -> count_lines ())

let () = Command.run command
