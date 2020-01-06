import pathlib

import jsonschema
import munch
import yaml

# TODO: покрыть тестами

_schema_path = pathlib.Path(__file__).absolute().parent / 'test-suite-schema.yml'
with _schema_path.open('r') as f:
    _TEST_SUITE_SCHEMA = yaml.full_load(f)


def load_config(config_path: pathlib.Path) -> list:
    with config_path.open('r') as f:
        config = yaml.full_load(f)
    config = _validate_and_set_defaults(config)
    return _extract_test_suites(config)


def _validate_and_set_defaults(config: dict):
    jsonschema.validate(instance=config, schema=_TEST_SUITE_SCHEMA)
    return config


def _extract_test_suites(config: dict):
    def apply_template(value, template):
        if value is None or template is None:
            return value or template
        if not isinstance(value, dict):
            return value

        assert isinstance(value, dict) and isinstance(template, dict)
        for key, template_value in template.items():
            value[key] = apply_template(value.get(key, None), template_value)
        return value

    template = config.get('testTemplate', None)
    test_suites = map(munch.munchify, config['tests'])
    test_suites = map(lambda test_suite: apply_template(test_suite, template), config['tests'])
    return list(test_suites)