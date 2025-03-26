use crate::*;
use crate::metta::*;
use crate::metta::text::Tokenizer;
use crate::metta::runner::str::*;
use super::{grounded_op, unit_result, regex};

use std::convert::TryInto;
use std::thread;

#[derive(Clone, Debug)]
pub struct DasOp {}


// DAS async

use tonic::{transport::Server, Request, Response, Status};

use das_proto::atom_space_node_server::{AtomSpaceNode, AtomSpaceNodeServer};
use das_proto::atom_space_node_client::AtomSpaceNodeClient;
use das_proto::{MessageData, Ack, Empty};

mod das_proto {
    tonic::include_proto!("dasproto"); // Matches the package name in your proto file
}

#[derive(Debug, Default)]
pub struct MyAtomSpaceNode {}

fn process_message(msg: MessageData) {

    println!("MessageData: {:?}", msg.args);


    match msg.command.as_ref() {
        "query_answer_tokens_flow" => println!("TOKENS FLOW"),
        "query_answer_flow" => println!("FLOW"),
        "query_answers_finished" => println!("FINISHED"),
        _ => println!("Unknwon: {:?}", msg.command),

    };
}

#[tonic::async_trait]
impl AtomSpaceNode for MyAtomSpaceNode {
    async fn execute_message(
        &self,
        request: Request<MessageData>,
    ) -> Result<Response<Empty>, Status> {
        // println!("Got a request: {:?}", request.into_inner());
        process_message(request.into_inner());

        Ok(Response::new(Empty{}))
    }

    async fn ping(&self, _request: Request<Empty>) -> Result<Response<Ack>, Status> {
        let reply = Ack { error: false, msg: "ack".into() };
        Ok(Response::new(reply))
    }

    // rpc ping (Empty) returns (Ack) {}
    // rpc execute_message (MessageData) returns (Empty) {}

}

async fn query() -> Result<(), Box<dyn std::error::Error>> {
    let mut client = AtomSpaceNodeClient::connect("http://[::1]:31700").await?;

    let query: Vec<String> = vec![
        "localhost:9090", "test", "false", "LINK_TEMPLATE", "Expression", "3", "NODE", "Symbol", "Similarity", "VARIABLE", "V1",
        "VARIABLE", "V2"
    ].iter().map(|&s| s.to_string()).collect();


    let request = tonic::Request::new(MessageData {
        // string command
        command: "pattern_matching_query".into(),
        // repeated string args
        args: query,
        // string sender
        sender: "RustMettaInterpreter".into(),
        // bool is_broadcast
        is_broadcast: false,
        // repeated string visited recipients
        visited_recipients: vec![],
    });

    let response = client.execute_message(request).await?;

    println!("Response: {:?}", response);

    Ok(())
}

fn send_query() {
    let rt = tokio::runtime::Runtime::new().unwrap();
    rt.block_on(async {
        let _ = query().await;
    });

}

// END DAS Async

grounded_op!(DasOp, "das!");

impl Grounded for DasOp {
    fn type_(&self) -> Atom {
        Atom::expr([ARROW_SYMBOL, ATOM_TYPE_UNDEFINED, UNIT_TYPE])
    }

    fn as_execute(&self) -> Option<&dyn CustomExecute> {
        Some(self)
    }
}

impl CustomExecute for DasOp {
    fn execute(&self, args: &[Atom]) -> Result<Vec<Atom>, ExecError> {
        let arg_error = || ExecError::from("das! expects single atom as an argument");
        let atom = args.get(0).ok_or_else(arg_error)?;
        println!("Sending Query to DAS");
        send_query();
        //TODO: Spawn a gRPC client to send a query to DAS
        unit_result()
    }
}


pub fn register_runner_tokens(tref: &mut Tokenizer) {
    let das_op = Atom::gnd(DasOp{});
    tref.register_token(regex(r"das!"), move |_| { das_op.clone() });
}

pub fn register_rust_stdlib_tokens(tref: &mut Tokenizer) {
    tref.register_token(regex(r#"(?s)^".*"$"#),
        |token| { let mut s = String::from(token); s.remove(0); s.pop(); Atom::gnd(Str::from_string(s)) });
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn das_op() {
        assert_eq!(DasOp{}.execute(&mut vec![sym!("A")]), unit_result());
    }
}

