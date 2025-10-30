use std::{
	collections::VecDeque,
	sync::{Arc, Mutex, OnceLock, RwLock},
};

use crate::types::BoxError;

static POOL: OnceLock<RwLock<PortPool>> = OnceLock::new();

#[derive(Debug, Default, Clone)]
pub struct PortPool {
	pool: Arc<Mutex<VecDeque<u16>>>,
	port_lower: u16,
	port_upper: u16,
}

impl PortPool {
	pub fn init(port_lower: u16, port_upper: u16) -> Result<(), BoxError> {
		let mut queue = VecDeque::with_capacity((port_upper - port_lower + 1) as usize);
		for port in port_lower..=port_upper {
			queue.push_back(port);
		}

		let pool = PortPool { pool: Arc::new(Mutex::new(queue)), port_lower, port_upper };
		match POOL.set(RwLock::new(pool)) {
			Ok(_) => Ok(()),
			Err(rwlock_with_new_pool) => {
				let new_pool = rwlock_with_new_pool.into_inner().unwrap();
				let lock = POOL.get().expect("PortPool::POOL not initialized!");
				let mut write_guard = lock.write().unwrap();
				*write_guard = new_pool;
				Ok(())
			},
		}
	}

	/// Retrieves a port from the pool.
	pub fn get_port() -> Result<u16, BoxError> {
		match POOL.get() {
			Some(locked_pool) => {
				let read_guard = locked_pool.read().unwrap();
				let err_msg = format!(
					"No port available in pool ({}-{})!",
					read_guard.port_lower, read_guard.port_upper
				);
				read_guard.clone().pool.lock().unwrap().pop_front().ok_or(BoxError::from(err_msg))
			},
			None => Err(BoxError::from("PortPool not initialized!")),
		}
	}

	/// Returns a port to the pool.
	pub fn return_port(port: u16) -> Result<(), BoxError> {
		match POOL.get() {
			Some(locked_pool) => {
				let write_guard = locked_pool.read().unwrap();
				write_guard.pool.lock().unwrap().push_front(port);
				Ok(())
			},
			None => Err(BoxError::from("PortPool not initialized!")),
		}
	}
}
