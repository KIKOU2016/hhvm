// Copyright (c) 2019, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the "hack" directory of this source tree.

extern crate crypto;
extern crate serde;
extern crate serde_json;

#[macro_use]
extern crate ocaml;

mod facts;
pub mod facts_parser;
pub mod facts_smart_constructors;
mod facts_smart_constructors_generated;
