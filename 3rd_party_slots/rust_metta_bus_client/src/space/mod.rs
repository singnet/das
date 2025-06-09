use std::{
	fmt::{Debug, Display},
	sync::{Arc, Mutex},
};

use hyperon_common::FlexRef;

use hyperon_atom::{matcher::BindingsSet, Atom};

use crate::{query_with_das, service_bus::ServiceBus};

use hyperon_space::{Space, SpaceCommon, SpaceEvent, SpaceMut, SpaceVisitor};

#[derive(Clone)]
pub struct DistributedAtomSpace {
	common: SpaceCommon,
	service_bus: Arc<Mutex<ServiceBus>>,
	name: Option<String>,
}

impl DistributedAtomSpace {
	pub fn new(service_bus: Arc<Mutex<ServiceBus>>, name: Option<String>) -> Self {
		Self { common: SpaceCommon::default(), service_bus, name }
	}

	pub fn query(&self, query: &Atom) -> BindingsSet {
		query_with_das(self.name.clone(), self.service_bus.clone(), query).unwrap()
	}

	pub fn add(&mut self, atom: Atom) {
		todo!()
	}

	pub fn remove(&mut self, atom: &Atom) -> bool {
		todo!()
	}

	pub fn replace(&mut self, from: &Atom, to: Atom) -> bool {
		todo!()
	}
}

impl Space for DistributedAtomSpace {
	fn common(&self) -> FlexRef<SpaceCommon> {
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
