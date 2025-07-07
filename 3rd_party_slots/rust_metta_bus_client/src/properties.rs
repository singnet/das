use std::collections::HashMap;

pub static ATTENTION_UPDATE_FLAG: &str = "attention_update_flag";
pub static COUNT_FLAG: &str = "count_flag";
pub static MAX_BUNDLE_SIZE: &str = "max_bundle_size";
pub static POSITIVE_IMPORTANCE_FLAG: &str = "positive_importance_flag";
pub static UNIQUE_ASSIGNMENT_FLAG: &str = "unique_assignment_flag";

#[derive(Clone, Debug, PartialEq)]
pub enum PropertyValue {
	String(String),
	Long(i64),
	UnsignedInt(u64),
	Double(f64),
	Bool(bool),
}

#[derive(Default, Clone)]
pub struct Properties(HashMap<String, PropertyValue>);

impl Properties {
	pub fn new() -> Self {
		Self(HashMap::new())
	}
	pub fn to_vec(&self) -> Vec<String> {
		let mut vec = vec![];
		for (k, v) in self.0.iter() {
			vec.push(k.clone());
			match v {
				PropertyValue::String(s) => vec.extend(["string".to_string(), s.clone()]),
				PropertyValue::Long(l) => vec.extend(["long".to_string(), format!("{}", l)]),
				PropertyValue::UnsignedInt(u) => {
					vec.extend(["unsigned_int".to_string(), format!("{}", u)])
				},
				PropertyValue::Double(d) => vec.extend(["double".to_string(), format!("{}", d)]),
				PropertyValue::Bool(b) => vec.extend(["bool".to_string(), format!("{}", b)]),
			}
		}
		vec.insert(0, vec.len().to_string());
		vec
	}
	pub fn insert(&mut self, key: String, value: PropertyValue) {
		self.0.insert(key, value);
	}
}
