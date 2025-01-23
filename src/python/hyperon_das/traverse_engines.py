from typing import TYPE_CHECKING, Any, Dict

from hyperon_das_atomdb import AtomDoesNotExist

from hyperon_das.cache import LocalIncomingLinks, RemoteIncomingLinks
from hyperon_das.cache.iterators import TraverseLinksIterator, TraverseNeighborsIterator

if TYPE_CHECKING:  # pragma no cover
    from hyperon_das.das import DistributedAtomSpace


class TraverseEngine:
    def __init__(self, handle: str, **kwargs) -> None:
        self.das: DistributedAtomSpace = kwargs['das']

        try:
            atom = self.das.get_atom(handle)
        except AtomDoesNotExist as e:
            raise e

        self._cursor = atom

    def get(self) -> Dict[str, Any]:
        """Returns the current cursor.

        Returns:
            Dict[str, Any]: The current cursor. A Python dict with all atom data.
        """
        return self._cursor

    def get_links(self, **kwargs) -> TraverseLinksIterator:
        """Returns all links that have the current cursor as one of their targets, that is, any links that point to the cursor.

        Keyword Args:
            link_type (str, optional): Filter links if named_type matches with this parameter.
            cursor_position (int, optional): Sets the position of the cursor, return the links after this position.
            target_type (str, optional):  Filter links if one of the targets matches with this parameter.
            filter (Callable[[Dict], bool], optional): Function used to filter the results after applying all other filters.
            chunk_size (int, optional): Chunk size. Defaults to 500.

        Returns:
            Iterator: An iterator that contains the links that match the criteria.

        Examples:
            >>> def has_score(atom):
                    if 'score' in atom and score > 0.5:
                        return True
                    return False
            >>> links = traverse_engine.get_links(
                    link_type='Ex',
                    cursor_position=2,
                    target_type='Sy',
                    filter=has_score
                )
            >>> next(links)
        """
        cursor, incoming_links = self.das.get_incoming_links(
            atom_handle=self._cursor['handle'],
            no_iterator=False,
            targets_document=True,
            cursor=0,
            chunk_size=kwargs.get('chunk_size', 500),
        )
        assert cursor == 0
        assert isinstance(incoming_links, (LocalIncomingLinks, RemoteIncomingLinks))
        return TraverseLinksIterator(source=incoming_links, cursor=self._cursor['handle'], **kwargs)

    def get_neighbors(self, **kwargs) -> TraverseNeighborsIterator:
        """Get all of "neighbors" that pointing to current cursor.
           Possible use cases to filter parameter:
            a. traverse.get_neighbors(..., filter=custom_filter)
                -> The custom_filter will be applied to Links
            b. traverse.get_neighbors(..., filter=(custom_filter1, custom_filter2))
                -> The custom_filter1 will be applied to Links and custom_filter2 will be applied to Targets
            c. traverse.get_neighbors(..., filter=(None, custom_filter2))
                -> The custom_filter2 will only be applied to Targets. This way there is no filter to Links
            d. traverse.get_neighbors(..., filter=(custom_filter1, None))
                -> The custom_filter1 will be applied to Links. This case is equal case `a`

        Keyword Args:
            link_type (str, optional): Filter links if named_type matches with this parameter.
            cursor_position (int, optional): Sets the position of the cursor, return the links after this position.
            target_type (str, optional):  Filter links if one of the targets matches with this parameter.
            filters (tuple[Callable[[dict], bool] | None, Callable[[dict], bool] | None], optional): Tuple containing
                filter function for links at pos 0 and filter function for targets at pos 1.
                Used to filter the results after applying all other filters.

        Returns:
            Iterator: An iterator that contains the neighbors that match the criteria.

        Examples:
            >>> neighbors = traverse_engine.get_neighbors(
                    link_type='Ex',
                    cursor_position=2,
                    target_type='Sy',
                    filter=(link_filter, target_filter)
                )
            >>> next(neighbors)
        """
        custom_filter = kwargs.pop('filters', None)
        filter_link = filter_target = None

        if custom_filter is not None and not isinstance(custom_filter, tuple):
            raise ValueError("The argument filters must be a tuple.")

        if isinstance(custom_filter, tuple):
            filter_link, filter_target = custom_filter

        if filter_link is not None:
            kwargs['filter'] = filter_link

        filtered_links = self.get_links(targets_only=True, **kwargs)
        return TraverseNeighborsIterator(source=filtered_links, filter=filter_target)

    def follow_link(self, **kwargs) -> Dict[str, Any]:
        """Update the current cursor by following the first of the neighbors that points to the current cursor.
           Possible use cases to filter parameter:
            a. traverse.get_neighbors(..., filter=custom_filter)
                -> The custom_filter will be applied to Links
            b. traverse.get_neighbors(..., filter=(custom_filter1, custom_filter2))
                -> The custom_filter1 will be applied to Links and custom_filter2 will be applied to Targets
            c. traverse.get_neighbors(..., filter=(None, custom_filter2))
                -> The custom_filter2 will only be applied to Targets. This way there is no filter to Links
            d. traverse.get_neighbors(..., filter=(custom_filter1, None))
                -> The custom_filter1 will be applied to Links. This case is equal case `a`

        Keyword Args:
            link_type (str, optional): Filter links if named_type matches with this parameter.
            cursor_position (int, optional): Sets the position of the cursor, return the links after this position.
            target_type (str, optional):  Filter links if one of the targets matches with this parameter.
            filters (tuple[Callable[[dict], bool] | None, Callable[[dict], bool] | None], optional): Tuple containing
                filter function for links at pos 0 and filter function for targets at pos 1.
                Used to filter the results after applying all other filters.
        Returns:
            Dict[str, Any]: The current cursor. A Python dict with all atom data.
        """
        filtered_neighbors = self.get_neighbors(**kwargs)
        if not filtered_neighbors.is_empty():
            self._cursor = filtered_neighbors.get()
        return self._cursor

    def goto(self, handle: str) -> Dict[str, Any]:
        """Reset current cursor to the passed handle.

        Args:
            handle (str): The handle of the atom to go to.

        Raises:
            AtomDoesNotExist: If the corresponding atom doesn't exist.

        Returns:
            Dict[str, Any]: The current cursor. A Python dict with all atom data.

        Examples:
            >>> traverse_engine.goto('asd1234567890')
            >>> {
                    'handle': 'asd1234567890',
                    'type': 'AI,
                    'composite_type_hash': 'd99asd1234567890',
                    'name': 'snet',
                    'named_type': 'AI'
                }
        """
        try:
            self._cursor = self.das.get_atom(handle)
        except AtomDoesNotExist as e:
            raise e
        return self._cursor
