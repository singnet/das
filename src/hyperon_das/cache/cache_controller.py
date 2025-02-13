from typing import Any, Dict, List, Optional

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway
from hyperon_das.context import Context
from hyperon_das.utils import QueryAnswer


class CacheController:
    """
    CacheController is the class used by query engines to interface with the caching sub-system.

    Local query engines feed the cache sub-system as queries are processed while remote query
    engines use it to check for query answers if they are available locally before forwarding
    these queries to a remote DAS.
    """

    def __init__(self, system_parameters: Dict[str, Any]):
        """
        System parameters are allowed to change dinamically after the CacheController
        object is created. So the behavior of the CacheController regarding each parameter
        may change accordingly. That's why individual system parameters are not stored individually
        as object fields. The whole passed system parameter's dict object is stored instead and
        checked everytime CacheController behavior is supposed to be controlled by some
        parameter. For example, the method enabled() will always check for the proper parameter
        in order to answer if CacheController is enabled or disabled.

        Args:
            system_parameters (Dict[str, Any], optional): Relevant parameters and their defaults:
            {
                'cache_enabled': False
            }
        """
        self.system_parameters = system_parameters
        self.atom_table = {}
        if self.enabled():
            self.attention_broker = AttentionBrokerGateway(system_parameters)

    def enabled(self):
        """
        Returns True iff the cache sub-system is enabled (as defined by a system parameter).
        """
        return self.system_parameters.get("cache_enabled")

    def regard_query_answer(self, query_answer: List[QueryAnswer]):
        """
        Feed this CacheController with the answers of a query made by an user. These answers are
        used to feed the cache itself and to allow the AttentionBroker to update its Hebbian
        Links properly.

        Args:
            query_answer (List[QueryANswer]): List of QueryAnswer objects.
        """
        if not self.enabled():
            return
        joint_count = {}
        for query_answer_object in query_answer:
            handle_set = query_answer_object.get_handle_set()
            self.attention_broker.correlate(handle_set)
            count = query_answer_object.get_handle_count()
            for key in count.keys():
                total = joint_count.get(key, 0) + count.get(key)
                joint_count[key] = total
        self.attention_broker.stimulate(joint_count)

    def add_context(self, context: Context):
        """
        Creates a new context to attach importance of atoms.

        Contexts are used by the AttentionBroker in order to keep different importance values for
        atoms in different contexts.

        Args:
            context (Context): new COntext object to be added.

        """
        if not self.enabled():
            return
        for query_answer in context.query_answers:
            self.regard_query_answer(query_answer)

    def get_atom(self, handle: str) -> Optional[Dict[str, Any]]:
        """
        Returns the corresponding atom if it's present in the local cache or None otherwise.

        Returns:
            Optional[Dict[str, Any]]: Atom document or None if the atom is not in local cache
        """
        return self.atom_table.get(handle, None)

    def get_atoms(self, handles: List[str]) -> List[Dict[str, Any]]:
        """
        Return a list of atoms given their handles.

        Args:
            handles (List[str]): List of handles.

        Returns:
            List[Dict[str, Any]]
        """
        raise NotImplementedError()
