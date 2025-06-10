use std::{
	env,
	process::exit,
	sync::{Arc, Mutex},
};

use hyperon_atom::Atom;
use metta_bus_client::{query, service_bus_singleton::ServiceBusSingleton, types::BoxError};

const MAX_QUERY_ANSWERS: u32 = 100;

fn main() -> Result<(), BoxError> {
	env_logger::init();

	// ./metta_bus_client localhost:11234 localhost:35700 1 LINK_TEMPLATE ...
	let args: Vec<String> = env::args().collect();

	if args.len() < 6 {
		println!(
			"Usage: {} \
			CLIENT_HOST:CLIENT_PORT \
			SERVER_HOST:SERVER_PORT \
			UPDATE_ATTENTION_BROKER \
			POSITIVE_IMPORTANCE \
			QUERY_TOKEN+ \
			(hosts are supposed to be public IPs or known hostnames)",
			&args[0]
		);
		exit(1)
	}

	let client_id = &args[1];
	let server_id = &args[2];

	let context = "";

	let update_attention_broker = &args[3] == "true" || &args[3] == "1";
	let positive_importance = &args[4] == "true" || &args[4] == "1";

	let unique_assignment = true;
	let count_only = false;

	let mut tokens_start_position = 5;
	let max_query_answers = match (args[5]).parse::<u32>() {
		Ok(value) => {
			tokens_start_position += 1;
			value
		},
		Err(_) => MAX_QUERY_ANSWERS,
	};

	log::info!("Using max_query_answers: {}", max_query_answers);

	let mut atoms = vec![];
	for arg in args.iter().skip(tokens_start_position) {
		if arg.starts_with("$") {
			atoms.push(Atom::var(arg.replace("$", "")));
		} else {
			atoms.push(Atom::sym(arg));
		}
	}

	ServiceBusSingleton::init(client_id.to_string(), server_id.to_string(), 64000, 64999)?;
	let service_bus = Arc::new(Mutex::new(ServiceBusSingleton::get_instance()));

	let bindings_set = query(
		Some(context.to_string()),
		service_bus,
		&Atom::expr(atoms),
		unique_assignment,
		positive_importance,
		update_attention_broker,
		count_only,
	)?;

	println!("{} answers:", bindings_set.len());
	for bs in bindings_set {
		println!("{}", bs);
	}

	Ok(())
}
