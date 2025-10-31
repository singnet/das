use std::sync::{OnceLock, RwLock};

use crate::{
	bus::{
		CONTEXT_CMD, INFERENCE_CMD, LINK_CREATION_CMD, PATTERN_MATCHING_QUERY_CMD,
		QUERY_EVOLUTION_CMD,
	},
	service_bus::ServiceBus,
	types::BoxError,
};

static SERVICE_BUS: OnceLock<RwLock<ServiceBus>> = OnceLock::new();

#[derive(Debug, Default)]
pub struct ServiceBusSingleton;

impl ServiceBusSingleton {
	pub fn init(
		host_id: String, known_peer: String, port_lower: u16, port_upper: u16,
	) -> Result<(), BoxError> {
		let service_bus = Self::internal_init(host_id, known_peer, port_lower, port_upper)?;

		match SERVICE_BUS.set(RwLock::new(service_bus)) {
			Ok(_) => Ok(()),
			Err(rwlock_with_new_bus) => {
				let new_service_bus = rwlock_with_new_bus.into_inner().unwrap();
				let lock = SERVICE_BUS.get().expect("ServiceBusSingleton not initialized!");
				let mut write_guard = lock.write().unwrap();
				*write_guard = new_service_bus;
				Ok(())
			},
		}
	}

	pub fn provide(
		host_id: String, known_peer: String, port_lower: u16, port_upper: u16,
	) -> Result<(), BoxError> {
		log::debug!(target: "das", "ServiceBusSingleton::provide(): Providing new service bus (host_id={host_id}, known_peer={known_peer}, port_lower={port_lower}, port_upper={port_upper})");
		let service_bus = Self::internal_init(host_id, known_peer, port_lower, port_upper)?;
		if let Some(lock) = SERVICE_BUS.get() {
			let mut write_guard = lock.write().unwrap();
			*write_guard = service_bus;
		} else {
			SERVICE_BUS
				.set(RwLock::new(service_bus))
				.expect("ServiceBusSingleton already initialized!");
		}
		Ok(())
	}

	pub fn get_instance() -> Result<ServiceBus, BoxError> {
		let lock = match SERVICE_BUS.get() {
			Some(lock) => lock,
			None => return Err(BoxError::from("ServiceBusSingleton not initialized!")),
		};
		let read_guard = lock.read().unwrap();
		Ok(read_guard.clone())
	}

	fn internal_init(
		host_id: String, known_peer: String, port_lower: u16, port_upper: u16,
	) -> Result<ServiceBus, BoxError> {
		let mut service_bus = ServiceBus::new(
			host_id,
			known_peer,
			vec![
				PATTERN_MATCHING_QUERY_CMD.to_string(),
				QUERY_EVOLUTION_CMD.to_string(),
				LINK_CREATION_CMD.to_string(),
				INFERENCE_CMD.to_string(),
				CONTEXT_CMD.to_string(),
			],
			port_lower,
			port_upper,
		)?;
		service_bus.join_network_thread()?;
		Ok(service_bus)
	}
}
