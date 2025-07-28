use std::{
	fmt::{Debug, Display},
	sync::{Arc, Mutex},
};

use hyperon_atom::{matcher::BindingsSet, Atom};
use hyperon_common::FlexRef;
use hyperon_space::{Space, SpaceCommon, SpaceMut, SpaceVisitor};

use crate::{init_service_bus, query_with_das, service_bus::ServiceBus, types::MeTTaRunner};

#[derive(Clone)]
pub struct DistributedAtomSpace {
	common: SpaceCommon,
	service_bus: Arc<Mutex<ServiceBus>>,
	name: Option<String>,
	metta_runner: MeTTaRunner,
}

impl DistributedAtomSpace {
	pub fn new(
		name: Option<String>, host_id: String, known_peer: String, port_lower: u16,
		port_upper: u16, metta_runner: MeTTaRunner,
	) -> Self {
		let service_bus = Arc::new(Mutex::new(
			init_service_bus(host_id, known_peer, port_lower, port_upper).unwrap(),
		));
		Self { common: SpaceCommon::default(), service_bus, name, metta_runner }
	}

	pub fn query(&self, query: &Atom) -> BindingsSet {
		query_with_das(
			self.name.clone(),
			self.service_bus.clone(),
			query,
			Some(self.metta_runner.clone()),
		)
		.unwrap()
	}

	pub fn add(&mut self, _atom: Atom) {
		todo!()
	}

	pub fn remove(&mut self, _atom: &Atom) -> bool {
		todo!()
	}

	pub fn replace(&mut self, _from: &Atom, _to: Atom) -> bool {
		todo!()
	}
}

impl Space for DistributedAtomSpace {
	fn common(&self) -> FlexRef<'_, SpaceCommon> {
		FlexRef::from_simple(&self.common)
	}
	fn query(&self, query: &Atom) -> BindingsSet {
		self.query(query)
	}
	fn atom_count(&self) -> Option<usize> {
		todo!()
	}
	fn visit(&self, _v: &mut dyn SpaceVisitor) -> Result<(), ()> {
		todo!()
	}
	fn subst(&self, pattern: &Atom, template: &Atom) -> Vec<Atom> {
		self.query(pattern)
			.drain(0..)
			.map(|bindings| {
				hyperon_atom::matcher::apply_bindings_to_atom_move(template.clone(), &bindings)
			})
			.collect()
	}
	fn as_any(&self) -> &dyn std::any::Any {
		self
	}
}

impl SpaceMut for DistributedAtomSpace {
	fn add(&mut self, atom: Atom) {
		self.add(atom)
	}
	fn remove(&mut self, atom: &Atom) -> bool {
		self.remove(atom)
	}
	fn replace(&mut self, from: &Atom, to: Atom) -> bool {
		self.replace(from, to)
	}
	fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
		todo!()
	}
}

impl Debug for DistributedAtomSpace {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match &self.name {
			Some(name) => write!(f, "DistributedAtomSpace-{name} ({self:p})"),
			None => write!(f, "DistributedAtomSpace-{self:p}"),
		}
	}
}

impl Display for DistributedAtomSpace {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match &self.name {
			Some(name) => write!(f, "DistributedAtomSpace-{name}"),
			None => write!(f, "DistributedAtomSpace-{self:p}"),
		}
	}
}
