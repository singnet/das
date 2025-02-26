import json  # noqa: F401
from unittest.mock import MagicMock, patch

import pytest
from requests import exceptions

import hyperon_das.link_filters as link_filter
from hyperon_das.client import FunctionsClient
from hyperon_das.exceptions import (
    FunctionsConnectionError,
    FunctionsTimeoutError,
    RequestError,
)
from hyperon_das.utils import serialize


class TestFunctionsClient:
    @pytest.fixture
    def mock_request(self):
        with patch("requests.sessions.Session.request") as mock_request:
            yield mock_request

    @pytest.fixture
    def client(self):
        with patch("hyperon_das.utils.check_server_connection", return_value=(200, "OK")):
            return FunctionsClient(host="0.0.0.0", port=1000)

    def test_get_atom_success(self, mock_request, client):
        expected_request_data = {"action": "get_atom", "input": {"handle": "123"}}
        expected_response = {
            "handle": "af12f10f9ae2002a1607ba0b47ba8407",
            "composite_type_hash": "d99a604c79ce3c2e76a2f43488d5d4c3",
            "name": "human",
            "named_type": "Concept",
        }

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)

        result = client.get_atom(handle="123")

        mock_request.assert_called_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_create_context_success(self, mock_request, client):
        expected_request_data = {
            "action": "create_context",
            "input": {"name": "n", "queries": []},
        }
        expected_response = {
            "name": "n",
            "handle": "h",
        }

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)

        result = client.create_context(name="n", queries=[])

        mock_request.assert_called_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_get_links_success(self, mock_request, client):
        expected_request_data = {
            "action": "get_links",
            "input": {
                "link_filter": {
                    "filter_type": link_filter.LinkFilterType.TARGETS,
                    "toplevel_only": False,
                    "link_type": "Inheritance",
                    "target_types": [],
                    "targets": [
                        "4e8e26e3276af8a5c2ac2cc2dc95c6d2",
                        "80aff30094874e75028033a38ce677bb",
                    ],
                }
            },
        }
        expected_response = [
            {
                "handle": "ee1c03e6d1f104ccd811cfbba018451a",
                "type": "Inheritance",
                "template": ["Inheritance", "Concept", "Concept"],
                "targets": [
                    "4e8e26e3276af8a5c2ac2cc2dc95c6d2",
                    "80aff30094874e75028033a38ce677bb",
                ],
            }
        ]

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)

        result = client.get_links(
            link_filter.Targets(
                [
                    "4e8e26e3276af8a5c2ac2cc2dc95c6d2",
                    "80aff30094874e75028033a38ce677bb",
                ],
                "Inheritance",
            )
        )

        mock_request.assert_called_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_query_success(self, mock_request, client):
        expected_request_data = {
            "action": "query",
            "input": {
                "query": {
                    "atom_type": "link",
                    "type": "Similarity",
                    "targets": [
                        {"atom_type": "node", "type": "Concept", "name": "human"},
                        {"atom_type": "node", "type": "Concept", "name": "monkey"},
                    ],
                },
                "parameters": [],
            },
        }
        expected_response = [
            {
                "handle": "bad7472f41a0e7d601ca294eb4607c3a",
                "type": "Similarity",
                "template": ["Similarity", "Concept", "Concept"],
                "targets": [
                    {
                        "handle": "af12f10f9ae2002a1607ba0b47ba8407",
                        "type": "Concept",
                        "name": "human",
                    },
                    {
                        "handle": "1cdffc6b0b89ff41d68bec237481d1e1",
                        "type": "Concept",
                        "name": "monkey",
                    },
                ],
            }
        ]

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)

        query = {
            "atom_type": "link",
            "type": "Similarity",
            "targets": [
                {"atom_type": "node", "type": "Concept", "name": "human"},
                {"atom_type": "node", "type": "Concept", "name": "monkey"},
            ],
        }

        result = client.query(query, parameters=[])

        mock_request.assert_called_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    @pytest.mark.parametrize(
        "query",
        [
            (
                {
                    "query": {
                        "atom_type": "link",
                        "targets": [
                            {"atom_type": "variable", "name": "v1"},
                            {
                                "atom_type": "node",
                                "type": "Symbol",
                                "name": '"earthworm"',
                            },
                            {"atom_type": "variable", "name": "v2"},
                        ],
                    },
                    "parameters": [],
                }
            ),
            (
                {
                    "query": {"atom_type": "link", "type": "Expression"},
                    "parameters": [],
                }
            ),
            (
                {
                    "query": {
                        "atom_type": "node",
                        "type": "Expression",
                        "targets": [
                            {"atom_type": "variable", "name": "v1"},
                            {
                                "atom_type": "node",
                                "type": "Symbol",
                                "name": '"earthworm"',
                            },
                            {"atom_type": "variable", "name": "v2"},
                        ],
                    },
                    "parameters": [],
                }
            ),
            (),
        ],
    )
    def test_query_malformed(self, query, mock_request, client):
        expected_request_data = {
            "action": "query",
            "input": {
                "query": query,
                "parameters": [],
            },
        }

        mock_response = MagicMock()
        mock_response.status_code = 400

        mock_request.return_value.content = serialize({})
        mock_request.return_value.raise_for_status.side_effect = exceptions.HTTPError(
            response=mock_response,
        )

        with pytest.raises(ValueError):
            client.query(query, parameters=[])

        mock_request.assert_called_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

    def test_count_atoms_success(self, mock_request, client):
        expected_request_data = {"action": "count_atoms", "input": {}}
        expected_response = (14, 26)

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)
        result = client.count_atoms()

        mock_request.assert_called_once_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_count_atoms_success_parameters(self, mock_request, client):
        values = {"parameters": {"context": "local"}}
        expected_request_data = {"action": "count_atoms", "input": values}
        expected_response = (14, 26)

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)
        result = client.count_atoms(values["parameters"])

        mock_request.assert_called_once_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_get_atoms_by_field(self, mock_request, client):
        query = [{"field": "name", "value": "test"}]
        expected_request_data = {
            "action": "get_atoms_by_field",
            "input": {"query": {k["field"]: k["value"] for k in query}},
        }
        expected_response = (14, 26)
        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)
        result = client.get_atoms_by_field(query=query)

        mock_request.assert_called_once_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_get_atoms_by_text_field(self, mock_request, client):
        expected_input = {"text_value": "value"}
        expected_request_data = {
            "action": "get_atoms_by_text_field",
            "input": expected_input,
        }
        expected_response = (14, 26)
        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)
        result = client.get_atoms_by_text_field(**expected_input)

        mock_request.assert_called_once_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_get_node_by_name_starting_with(self, mock_request, client):
        expected_input = {
            "node_type": "Concept",
            "startswith": "nam",
        }
        expected_request_data = {
            "action": "get_node_by_name_starting_with",
            "input": expected_input,
        }
        expected_response = (14, 26)
        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)
        result = client.get_node_by_name_starting_with(**expected_input)

        mock_request.assert_called_once_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(expected_request_data),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_send_request_success(self, mock_request, client):
        payload = {"action": "get_atom", "input": {"handle": "123"}}
        expected_response = {
            "handle": "af12f10f9ae2002a1607ba0b47ba8407",
            "composite_type_hash": "d99a604c79ce3c2e76a2f43488d5d4c3",
            "name": "human",
            "named_type": "Concept",
        }

        mock_request.return_value.status_code = 200
        mock_request.return_value.content = serialize(expected_response)

        result = client._send_request(payload)

        mock_request.assert_called_with(
            method="POST",
            url="http://0.0.0.0:1000/function/query-engine",
            data=serialize(payload),
            headers={"Content-Type": "application/octet-stream"},
        )

        assert result == expected_response

    def test_send_request_connection_error(self, mock_request, client):
        mock_request.side_effect = exceptions.ConnectionError()

        payload = {"action": "get_atom", "input": {"handle": "123"}}

        with pytest.raises(FunctionsConnectionError):
            client._send_request(payload)

    def test_send_request_timeout_error(self, mock_request, client):
        mock_request.side_effect = exceptions.Timeout()

        payload = {"action": "get_atom", "input": {"handle": "123"}}

        with pytest.raises(FunctionsTimeoutError):
            client._send_request(payload)

    def test_send_request_request_exception(self, mock_request, client):
        mock_request.side_effect = exceptions.RequestException()

        payload = {"action": "get_atom", "input": {"handle": "123"}}

        with pytest.raises(RequestError):
            client._send_request(payload)
