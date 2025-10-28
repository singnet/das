use std::collections::HashMap;
use std::fmt::{Display, Formatter};

pub static CONTEXT: &str = "context";

// Networking
pub static HOSTNAME: &str = "hostname";
pub static PORT_LOWER: &str = "port_lower";
pub static PORT_UPPER: &str = "port_upper";
pub static KNOWN_PEER_ID: &str = "known_peer_id";

pub static ATTENTION_UPDATE_FLAG: &str = "attention_update_flag";
pub static COUNT_FLAG: &str = "count_flag";
pub static MAX_BUNDLE_SIZE: &str = "max_bundle_size";
pub static MAX_ANSWERS: &str = "max_answers";
pub static POSITIVE_IMPORTANCE_FLAG: &str = "positive_importance_flag";
pub static UNIQUE_ASSIGNMENT_FLAG: &str = "unique_assignment_flag";
pub static POPULATE_METTA_MAPPING: &str = "populate_metta_mapping";
pub static USE_METTA_AS_QUERY_TOKENS: &str = "use_metta_as_query_tokens";

// Evolution
pub static POPULATION_SIZE: &str = "population_size";
pub static MAX_GENERATIONS: &str = "max_generations";
pub static ELITISM_RATE: &str = "elitism_rate";
pub static SELECTION_RATE: &str = "selection_rate";
pub static TOTAL_ATTENTION_TOKENS: &str = "total_attention_tokens";

// Context Broker
pub static USE_CACHE: &str = "use_cache";
pub static ENFORCE_CACHE_RECREATION: &str = "enforce_cache_recreation";
pub static INITIAL_RENT_RATE: &str = "initial_rent_rate";
pub static INITIAL_SPREADING_RATE_LOWERBOUND: &str = "initial_spreading_rate_lowerbound";
pub static INITIAL_SPREADING_RATE_UPPERBOUND: &str = "initial_spreading_rate_upperbound";

#[derive(Clone, Debug, PartialEq)]
pub enum PropertyValue {
	String(String),
	Long(i64),
	UnsignedInt(u64),
	Double(f64),
	Bool(bool),
}

impl PropertyValue {
	pub fn parse_str(s: &str) -> Result<Self, String> {
		match s {
			"true" => Ok(Self::Bool(true)),
			"false" => Ok(Self::Bool(false)),
			s if s.parse::<u64>().is_ok() => Ok(Self::UnsignedInt(s.parse::<u64>().unwrap())),
			s if s.parse::<i64>().is_ok() => Ok(Self::Long(s.parse::<i64>().unwrap())),
			s if s.parse::<f64>().is_ok() => Ok(Self::Double(s.parse::<f64>().unwrap())),
			s => Ok(Self::String(s.to_string())),
		}
	}

	pub fn to_string(&self) -> String {
		match self {
			Self::String(s) => s.clone(),
			Self::Long(l) => l.to_string(),
			Self::UnsignedInt(u) => u.to_string(),
			Self::Double(d) => d.to_string(),
			Self::Bool(b) => b.to_string(),
		}
	}
}

#[derive(Clone)]
pub struct Properties(HashMap<String, PropertyValue>);

impl Properties {
	pub fn new() -> Self {
		Self(HashMap::new())
	}

	pub fn to_vec(&self) -> Vec<String> {
		let mut vec = vec![];
		for (k, v) in self.0.iter() {
			vec.push(k.to_string());
			match v {
				PropertyValue::String(s) => vec.extend(["string".to_string(), s.clone()]),
				PropertyValue::Long(l) => vec.extend(["long".to_string(), format!("{l}")]),
				PropertyValue::UnsignedInt(u) => {
					vec.extend(["unsigned_int".to_string(), format!("{u}")])
				},
				PropertyValue::Double(d) => vec.extend(["double".to_string(), format!("{d}")]),
				PropertyValue::Bool(b) => vec.extend(["bool".to_string(), format!("{b}")]),
			}
		}
		vec.insert(0, vec.len().to_string());
		vec
	}

	pub fn insert(&mut self, key: String, value: PropertyValue) {
		self.0.insert(key, value);
	}

	pub fn get<T: TryFrom<PropertyValue>>(&self, key: &str) -> T
	where
		T::Error: std::fmt::Debug,
	{
		T::try_from(self.0.get(key).unwrap().clone()).unwrap()
	}

	pub fn try_get(&self, key: &str) -> Option<&PropertyValue> {
		self.0.get(key)
	}

	pub fn keys(&self) -> Vec<String> {
		let mut keys = self.0.keys().cloned().collect::<Vec<String>>();
		keys.sort();
		keys
	}

	pub fn section_print(&self) {
		println!("DAS Parameters:");
		println!("  Networking:");
		println!("    {}", self.print_with_set_param(HOSTNAME).unwrap());
		println!("    {}", self.print_with_set_param(PORT_LOWER).unwrap());
		println!("    {}", self.print_with_set_param(PORT_UPPER).unwrap());
		println!("    {}", self.print_with_set_param(KNOWN_PEER_ID).unwrap());
		println!("  Context:");
		println!("    {}", self.print_with_set_param(CONTEXT).unwrap());
		println!("    {}", self.print_with_set_param(USE_CACHE).unwrap());
		println!("    {}", self.print_with_set_param(ENFORCE_CACHE_RECREATION).unwrap());
		println!("    {}", self.print_with_set_param(INITIAL_RENT_RATE).unwrap());
		println!("    {}", self.print_with_set_param(INITIAL_SPREADING_RATE_LOWERBOUND).unwrap());
		println!("    {}", self.print_with_set_param(INITIAL_SPREADING_RATE_UPPERBOUND).unwrap());
		println!("  Query:");
		println!("    {}", self.print_with_set_param(MAX_ANSWERS).unwrap());
		println!("    {}", self.print_with_set_param(MAX_BUNDLE_SIZE).unwrap());
		println!("    {}", self.print_with_set_param(UNIQUE_ASSIGNMENT_FLAG).unwrap());
		println!("    {}", self.print_with_set_param(POSITIVE_IMPORTANCE_FLAG).unwrap());
		println!("    {}", self.print_with_set_param(ATTENTION_UPDATE_FLAG).unwrap());
		println!("    {}", self.print_with_set_param(COUNT_FLAG).unwrap());
		println!("    {}", self.print_with_set_param(POPULATE_METTA_MAPPING).unwrap());
		println!("    {}", self.print_with_set_param(USE_METTA_AS_QUERY_TOKENS).unwrap());
		println!("  Evolution:");
		println!("    {}", self.print_with_set_param(POPULATION_SIZE).unwrap());
		println!("    {}", self.print_with_set_param(MAX_GENERATIONS).unwrap());
		println!("    {}", self.print_with_set_param(ELITISM_RATE).unwrap());
		println!("    {}", self.print_with_set_param(SELECTION_RATE).unwrap());
		println!("    {}", self.print_with_set_param(TOTAL_ATTENTION_TOKENS).unwrap());
	}

	fn print_with_set_param(&self, key: &str) -> Result<String, String> {
		let value = self.try_get(key).ok_or(format!("Property value not found: {key}"))?;
		Ok(format!("!(das-set-param! ({} {}))", key, value.to_string()))
	}
}

impl Default for Properties {
	fn default() -> Self {
		let mut map = HashMap::new();

		map.insert(CONTEXT.to_string(), PropertyValue::String(String::from("context")));

		// Networking
		map.insert(HOSTNAME.to_string(), PropertyValue::String(String::new()));
		map.insert(PORT_LOWER.to_string(), PropertyValue::UnsignedInt(0));
		map.insert(PORT_UPPER.to_string(), PropertyValue::UnsignedInt(0));
		map.insert(KNOWN_PEER_ID.to_string(), PropertyValue::String(String::new()));

		// Query
		map.insert(MAX_ANSWERS.to_string(), PropertyValue::UnsignedInt(1_000));
		map.insert(MAX_BUNDLE_SIZE.to_string(), PropertyValue::UnsignedInt(1_000));
		map.insert(UNIQUE_ASSIGNMENT_FLAG.to_string(), PropertyValue::Bool(true));
		map.insert(POSITIVE_IMPORTANCE_FLAG.to_string(), PropertyValue::Bool(false));
		map.insert(ATTENTION_UPDATE_FLAG.to_string(), PropertyValue::Bool(false));
		map.insert(COUNT_FLAG.to_string(), PropertyValue::Bool(false));
		map.insert(POPULATE_METTA_MAPPING.to_string(), PropertyValue::Bool(true));
		map.insert(USE_METTA_AS_QUERY_TOKENS.to_string(), PropertyValue::Bool(true));

		// Evolution
		map.insert(POPULATION_SIZE.to_string(), PropertyValue::UnsignedInt(50));
		map.insert(MAX_GENERATIONS.to_string(), PropertyValue::UnsignedInt(10));
		map.insert(ELITISM_RATE.to_string(), PropertyValue::Double(0.08));
		map.insert(SELECTION_RATE.to_string(), PropertyValue::Double(0.1));
		map.insert(TOTAL_ATTENTION_TOKENS.to_string(), PropertyValue::UnsignedInt(100_000));

		// Context Broker
		map.insert(USE_CACHE.to_string(), PropertyValue::Bool(true));
		map.insert(ENFORCE_CACHE_RECREATION.to_string(), PropertyValue::Bool(false));
		map.insert(INITIAL_RENT_RATE.to_string(), PropertyValue::Double(0.25));
		map.insert(INITIAL_SPREADING_RATE_LOWERBOUND.to_string(), PropertyValue::Double(0.50));
		map.insert(INITIAL_SPREADING_RATE_UPPERBOUND.to_string(), PropertyValue::Double(0.70));

		Self(map)
	}
}

impl TryFrom<PropertyValue> for u64 {
	type Error = String;
	fn try_from(value: PropertyValue) -> Result<Self, Self::Error> {
		match value {
			PropertyValue::UnsignedInt(u) => Ok(u),
			_ => Err(format!("Failed to convert property value to u64: {value:?}")),
		}
	}
}

impl TryFrom<PropertyValue> for bool {
	type Error = String;
	fn try_from(value: PropertyValue) -> Result<Self, Self::Error> {
		match value {
			PropertyValue::Bool(b) => Ok(b),
			_ => Err(format!("Failed to convert property value to bool: {value:?}")),
		}
	}
}

impl TryFrom<PropertyValue> for f64 {
	type Error = String;
	fn try_from(value: PropertyValue) -> Result<Self, Self::Error> {
		match value {
			PropertyValue::Double(d) => Ok(d),
			_ => Err(format!("Failed to convert property value to f64: {value:?}")),
		}
	}
}

impl TryFrom<PropertyValue> for String {
	type Error = String;
	fn try_from(value: PropertyValue) -> Result<Self, Self::Error> {
		match value {
			PropertyValue::String(s) => Ok(s),
			_ => Err(format!("Failed to convert property value to String: {value:?}")),
		}
	}
}

impl Display for Properties {
	fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
		for (k, v) in self.0.iter() {
			write!(f, "\"{k}\": {v:?}")?;
			writeln!(f)?;
		}
		Ok(())
	}
}
