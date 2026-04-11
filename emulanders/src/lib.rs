#![no_std]

#[macro_use]
extern crate nx;

extern crate static_assertions;

extern crate alloc;
extern crate paste;

#[macro_use]
pub mod logger;

pub mod rc;

#[macro_use]
pub mod fsext;

pub mod ipc;

pub mod emu;

pub mod skylander;

pub use ipc::emu::{EmulationService, IEmulationServiceClient};
