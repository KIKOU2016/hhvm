(**
 * Copyright (c) 2018, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 *)
open Core_kernel
open Aast
open Typing_defs
open Utils

module S = Core_kernel.Sequence
module Cls = Decl_provider.Class
module Env = Tast_env

let collect_attrs_from_ty_sid ?(include_optional=false) env add bag sid =
  match Env.get_class env sid with
  | None -> bag
  | Some c ->
    let should_collect {ce_xhp_attr; _} = (match ce_xhp_attr with
    | Some {xa_has_default; xa_tag = None} when include_optional ->
      xa_has_default
    | Some {xa_tag = Some Required; _} -> true
    | _ -> false) in
    let required_attrs = S.filter (Cls.props c) (compose should_collect snd) in
    S.fold required_attrs ~init:bag ~f:(fun s (n, elt) -> add (n, elt.ce_origin) s)

let rec collect_attrs_from_ty env set ty =
  let _, ty = Env.expand_type env ty in
  let _, ty = Env.fold_unresolved env ty in
  match ty with
  | _, Tunion (ty :: tys) ->
    let collect = collect_attrs_from_ty env SSet.empty in
    List.fold (List.map tys collect)  ~init:(collect ty) ~f:SSet.inter
  | _, Tclass ((_, sid), _, _) ->
    collect_attrs_from_ty_sid ~include_optional:true env
      (compose SSet.add fst) set sid
  | _ -> set

let collect_attrs env attrs =
  let collect_attr set attr = match attr with
  | Xhp_simple ((_, n), _) -> SSet.add (":" ^ n) set
  | Xhp_spread ((_, ty), _) -> collect_attrs_from_ty env set ty in
  List.fold attrs ~init:SSet.empty ~f:collect_attr

let check_attrs pos env sid attrs =
  let collect_with_ty = collect_attrs_from_ty_sid env (fun (n, c) -> SMap.add n c) in
  let required_attrs = collect_with_ty SMap.empty sid in
  let supplied_attrs = collect_attrs env attrs in
  let missing_attrs = SSet.fold SMap.remove supplied_attrs required_attrs in
  if SMap.is_empty missing_attrs then ()
  else SMap.iter (fun attr origin_sid ->
    let msg = match Env.get_class env origin_sid with
    | None -> []
    | Some ty -> Reason.to_string
      ("The attribute " ^ attr ^ " is declared in this class.")
      (Reason.Rwitness (Cls.pos ty))
    in
    Errors.missing_xhp_required_attr pos attr msg
  ) missing_attrs

let make_handler opts =
  let handler = object
    inherit Tast_visitor.handler_base

    method! at_expr env = function
    | ((pos, _), Xml ((_, sid), attrs, _)) -> check_attrs pos env sid attrs
    | _ -> ()
  end
  in
  if TypecheckerOptions.check_xhp_attribute opts
  then Some handler
  else None
