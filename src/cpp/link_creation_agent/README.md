# DAS Link Creation Agent


<!-- ![alt](doc/assets/das_link_creation_diagram.png.png) -->

DAS Link Creation Agent (DAS LCA), process a query and create links using the result of the query and a custom template. The service can execute n times a request to update the database with relevant links over the time.

![alt](doc/assets/das_link_creation_hla.png.png)

Query Example

```
LINK_TEMPLATE Expression 3 
    NODE Symbol feature_id 
    LINK_TEMPLATE Expression 2 
        NODE Symbol member 
        VARIABLE V1 
        LINK_TEMPLATE Expression 2 
            NODE Symbol feature 
            VARIABLE V2
```

Link Template Example:

```
LINK_CREATE Similarity 2
    VARIABLE V1 
    VARIABLE V2
```


## How to build


## Development