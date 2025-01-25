import pytest

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway


class TestAttentionBrokerGateway:
    def test_creation(self):
        with pytest.raises(ValueError):
            AttentionBrokerGateway({})
        with pytest.raises(ValueError):
            AttentionBrokerGateway({'attention_broker_hostname': 'localhost'})
        with pytest.raises(ValueError):
            AttentionBrokerGateway({'attention_broker_port': 27000})
        # successful creation is tested in a integration test because it requires
        # actual connection to a GRPC server
