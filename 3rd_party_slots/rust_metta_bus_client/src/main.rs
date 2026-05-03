use std::{
	env,
	process::exit,
	sync::{Arc, Mutex},
	thread::sleep,
	time::Duration,
};

use hyperon_atom::Atom;
use metta_bus_client::{
	bus::PATTERN_MATCHING_QUERY_CMD,
	extract_query_params,
	helpers::query_answer::query_answer_to_bindings,
	host_id_from_atom, pattern_matching_query,
	properties::{self, Properties, PropertyValue},
	service_bus_singleton::ServiceBusSingleton,
	types::BoxError,
	QueryType,
};

const MAX_QUERY_ANSWERS: u64 = 100;

fn main() -> Result<(), BoxError> {
	env_logger::init();

	// ./metta_bus_client localhost:52000-52999 localhost:35700 1 0 'LINK_TEMPLATE ...'
	// ./metta_bus_client localhost:52000-52999 localhost:35700 0 0 '(Similarity "human" $S)'
	let args: Vec<String> = env::args().collect();

	if args.len() < 6 {
		println!(
			"Usage: {} \
			CLIENT_HOST:PORT_LOWER-PORT_UPPER \
			SERVER_HOST:SERVER_PORT \
			UPDATE_ATTENTION_BROKER \
			POSITIVE_IMPORTANCE \
			QUERY_TOKEN+ \
			(hosts are supposed to be public IPs or known hostnames)",
			&args[0]
		);
		exit(1)
	}

	let (host_id, port_lower, port_upper) =
		host_id_from_atom(&Atom::sym(&args[1])).map_err(|e| format!("{e:?}"))?;
	let (known_peer, _, _) =
		host_id_from_atom(&Atom::sym(&args[2])).map_err(|e| format!("{e:?}"))?;

	let context = "context".to_string();

	let update_attention_broker = args[3].parse::<u64>().unwrap_or(0);
	let positive_importance = &args[4] == "true" || &args[4] == "1";

	let mut props = Properties::default();
	props.insert(properties::CONTEXT.to_string(), PropertyValue::String(context));
	props.insert(
		properties::ATTENTION_UPDATE.to_string(),
		PropertyValue::UnsignedInt(update_attention_broker),
	);
	props.insert(
		properties::POSITIVE_IMPORTANCE_FLAG.to_string(),
		PropertyValue::Bool(positive_importance),
	);

	let mut tokens_start_position = 5;
	let max_answers = match (args[5]).parse::<u64>() {
		Ok(value) => {
			tokens_start_position += 1;
			value
		},
		Err(_) => MAX_QUERY_ANSWERS,
	};

	props.insert(properties::MAX_ANSWERS.to_string(), PropertyValue::UnsignedInt(max_answers));
	log::info!("Using max_answers: {max_answers}");

	let tokens = args
		.iter()
		.skip(tokens_start_position)
		.map(|s| s.to_string())
		.collect::<Vec<String>>()
		.join(" ");

	ServiceBusSingleton::init(host_id, known_peer, port_lower, port_upper)?;
	let service_bus = ServiceBusSingleton::get_instance()?;
	let mut bus = service_bus.bus_node.lock().unwrap().bus.clone();

	let mut timeout = 0;
	while bus.get_ownership(PATTERN_MATCHING_QUERY_CMD.to_string()).is_empty() && timeout < 100 {
		timeout += 1;
		drop(bus);
		sleep(Duration::from_millis(100));
		bus = service_bus.bus_node.lock().unwrap().bus.clone();
	}

	if bus.get_ownership(PATTERN_MATCHING_QUERY_CMD.to_string()).is_empty() {
		return Err("No owner found for pattern_matching_query command".into());
	}

	let params = match extract_query_params(&QueryType::String(tokens), props) {
		Ok(params) => params,
		Err(_) => return Err("Error extracting query params".into()),
	};

	let query_answers = pattern_matching_query(Arc::new(Mutex::new(service_bus)), &params)?;

	println!("{} answers:", query_answers.len());
	for query_answer in query_answers {
		if query_answer.handles.len() == 1 {
			let bindings = query_answer_to_bindings(&query_answer, true)?;
			println!("{bindings:?}");
		} else {
			println!("{query_answer:?}");
		}
	}

	Ok(())
}
