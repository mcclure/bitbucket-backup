open OUnit2

let test_ppx_const _ =
  (* notice none of these tests will even compile if if%const isn't working *)
  assert_equal "BLARG"    @@ if false then "WRONG" else if%const 3=3 then "BLARG" else 3;
  assert_equal "BLAAAARG" @@ if false then "WRONG" else if%const 3=4 then 4 else "BLAAAARG";
  assert_equal "...blarg" @@ if%const true then (if%const false then 5 else "...blarg") else 3;
  assert_equal "Blarg."   @@ if%const 2=if%const 1=0 then 3 else 2 then "Blarg." else 1;
  assert_equal "Blarg"    @@ if%const 3 <> 3 then 4 else if%const 4 <> 3 then "Blarg" else 6

let suite = "Test ppx_const" >::: [
    "test_ppx_const" >:: test_ppx_const;
  ]

let _ =
  run_test_tt_main suite
