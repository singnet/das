from diagrams import Diagram, Cluster, Edge, Node
from diagrams.programming import language, framework, flowchart
from diagrams.onprem.compute import Server
from diagrams.onprem.client import Client
from diagrams.onprem.database import PostgreSQL
import copy

from diagrams.c4 import Container, C4Node, Person, SystemBoundary, Relationship
import json


query = """
Query Example

LINK_TEMPLATE Expression 3 
    NODE Symbol feature_id 
    LINK_TEMPLATE Expression 2 
        NODE Symbol member 
        VARIABLE V1 
        LINK_TEMPLATE Expression 2 
            NODE Symbol feature 
            VARIABLE V2

"""


link_creation = """ 
Link Template Example

LINK_CREATE Expression 2
    VARIABLE V1 
    VARIABLE V2

"""

node_attributes = {
        "label": "", 
        "direction": "RL",
        "shape": "rect",
        "width": "1",
        "height": "1",
        "fixedsize": "false",
        "labeljust": "l",
        "style": "filled",
        "fillcolor": "dodgerblue3",
        "fontcolor": "white",
    }


# with Diagram("DAS Link Creation", filename="doc/assets/das_link_creation_diagram.png", show=False):
#     client = Client("DAS Server")
#     das_link_creation_node = Server("DAS Link Creation Node")
#     das_query_broker = Server("DAS Query Broker")
#     edges = [
#         Edge(label="Create Links (query_str, max_resp, link_creation_type, link_creation_template)"),
#         Edge(label="Query"),
#         Edge(label="Query Response"),
#         Edge(label="Create Link"),
#     ]


#     client >> edges[3] >> das_link_creation_node
#     das_link_creation_node >> edges[2] >> das_query_broker
#     das_query_broker >> edges[1] >> das_link_creation_node
#     das_link_creation_node >> edges[0] >> client

with Diagram("DAS Link Creation", filename="doc/assets/das_link_creation_hla", show=False):
    client = Person(
        name="DAS Server", description="Sends a Link creation request"
    )

    with SystemBoundary("DAS Link Creation Agent"):
        das_node_server = Container(name="DAS Node Server", technology="DAS NODE", description="Receives a link creation request")
        link_creation_agent = Container(name="DAS LCA", technology="C++ Core", description="Process a link creation request")
        link_creation_agent_service = Container(name="DAS LCA", technology="C++ Service", description="Process multiple queries")
        das_node_client = Container(name="DAS Node Client", technology="DAS NODE", description="Sends atoms creation requests")
        das_node_client2 = Container(name="DAS Node Client", technology="DAS NODE", description="Sends query requests")
        

    das_query_agent = Container(name="DAS Query Agent", technology="DAS NODE", description="Process Template Queries")

    das_node_client >> Relationship("Create links and nodes") >> client
    client >> Relationship("Request Link Creation") >> das_node_server

    das_node_server >> Relationship("Start Link Creation") >> link_creation_agent
    link_creation_agent >> das_node_client2 << link_creation_agent
    das_node_client2 >> Relationship("Request Query template results") >> das_query_agent
    
    das_query_agent >> Relationship("Returns a Query Iterator") >> das_node_client2
    link_creation_agent >> Relationship("Spawn a LCA service") >> link_creation_agent_service
    link_creation_agent_service >> Relationship("Send create atom requests") >> das_node_client


