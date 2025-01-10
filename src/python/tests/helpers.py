from copy import deepcopy
from typing import TypeAlias

from hyperon_das_atomdb.database import AtomDB, LinkT, NodeT

CustomAttributesT: TypeAlias = dict[str, str | int | float | bool]


def check_handle(handle):
    return all((isinstance(handle, str), len(handle) == 32, int(handle, 16)))


def add_node(
    db: AtomDB,
    node_name: str,
    node_type: str,
    adapter: str,
    custom_attributes: CustomAttributesT = {},
) -> NodeT:
    node_params = NodeT(type=node_type, name=node_name, custom_attributes=custom_attributes)
    node = db.add_node(node_params)
    if adapter != "in_memory_db":
        db.commit()
    return node


def add_link(
    db: AtomDB,
    link_type: str,
    targets: list[NodeT | LinkT],
    adapter: str,
    is_toplevel: bool = True,
    custom_attributes: CustomAttributesT = {},
) -> LinkT:
    link = db.add_link(
        LinkT(
            type=link_type,
            targets=targets,
            custom_attributes=custom_attributes,
        ),
        toplevel=is_toplevel,
    )
    if adapter != "in_memory_db":
        db.commit()
    return link


def dict_to_node_params(node_dict: dict) -> NodeT:
    return NodeT(**node_dict)


def dict_to_link_params(link_dict: dict) -> LinkT:
    targets = [
        dict_to_link_params(target) if "targets" in target else dict_to_node_params(target)
        for target in link_dict["targets"]
    ]
    params = deepcopy(link_dict)
    params.update({"targets": targets})
    try:
        return LinkT(**params)
    except TypeError as ex:
        raise AssertionError(f"{type(ex)}: {ex} - {params=}")
