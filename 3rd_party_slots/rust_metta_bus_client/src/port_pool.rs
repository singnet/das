use std::{
	collections::VecDeque,
	sync::{Arc, Mutex, OnceLock},
};

use crate::types::BoxError;

static INITIALIZED: OnceLock<bool> = OnceLock::new();
static POOL: OnceLock<Arc<Mutex<VecDeque<u16>>>> = OnceLock::new();
static PORT_LOWER: OnceLock<u16> = OnceLock::new();
static PORT_UPPER: OnceLock<u16> = OnceLock::new();

#[derive(Debug, Default, Clone)]
pub struct PortPool;

impl PortPool {
	pub fn initialize_statics(port_lower: u16, port_upper: u16) -> Result<(), BoxError> {
		INITIALIZED.set(true).expect("PortPool already initialized!");

		let mut queue = VecDeque::with_capacity((port_upper - port_lower + 1) as usize);
		for port in port_lower..=port_upper {
			queue.push_back(port);
		}

		POOL.set(Arc::new(Mutex::new(queue))).expect("PortPool::POOL already initialized!");
		PORT_LOWER.set(port_lower).expect("PortPool::PORT_LOWER already initialized!");
		PORT_UPPER.set(port_upper).expect("PortPool::PORT_UPPER already initialized!");

		Ok(())
	}

	/// Retrieves a port from the pool.
	pub fn get_port() -> u16 {
		INITIALIZED.get().expect("PortPool not initialized!");
		let locked_pool = POOL.get().unwrap();
		match locked_pool.lock() {
			Ok(mut pool) => pool.pop_front().unwrap(),
			Err(_) => panic!(
				"Unable to get available PORT number in PortPool({}..{})",
				PORT_LOWER.get().unwrap(),
				PORT_LOWER.get().unwrap()
			),
		}
	}

	/// Returns a port to the pool.
	pub fn return_port(port: u16) {
		INITIALIZED.get().expect("PortPool not initialized!");
		let locked_pool = POOL.get().unwrap();
		match locked_pool.lock() {
			Ok(mut pool) => pool.push_front(port),
			Err(_) => panic!(
				"Unable to return PORT {} to PortPool({}..{})",
				port,
				PORT_LOWER.get().unwrap(),
				PORT_LOWER.get().unwrap()
			),
		}
	}
}
