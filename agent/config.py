import pathlib

import yaml
from jsonschema import Draft7Validator, validators
from munch import munchify

# TODO: покрыть тестами

_schema_path = pathlib.Path(__file__).absolute().parent / 'test-suite-schema.yml'
with _schema_path.open('r') as f:
    _TEST_SUITE_SCHEMA = yaml.full_load(f)


def load_config(config_path: pathlib.Path) -> list:
    """
    Load test definitions from config.

    :param config_path: path to the file containing test suite configuration
    :return: list of Munch objects containing test definitions as they are specified in the configuration
    """
    with config_path.open('r') as f:
        config = yaml.full_load(f)
    config['tests'] = [_apply_template(test, config.get('testTemplate', None)) for test in config.get('tests', [])]
    config = _set_defaults_and_validate(config)
    return munchify(config).tests


def _apply_template(value, template):
    if value is None or template is None:
        return value or template
    if not isinstance(value, dict):
        return value

    assert isinstance(value, dict) and isinstance(template, dict)
    for key, template_value in template.items():
        value[key] = _apply_template(value.get(key, None), template_value)
    return value


def _set_defaults_and_validate(config: dict):
    base_validator = Draft7Validator
    validate_properties = base_validator.VALIDATORS["properties"]

    def set_defaults_and_validate_properties(validator, properties, instance, schema):
        for prop, subschema in properties.items():
            if 'default' in subschema:
                instance.setdefault(prop, subschema['default'])
        for error in validate_properties(validator, properties, instance, schema):
            yield error

    validator = validators.extend(base_validator, validators={"properties": set_defaults_and_validate_properties})
    validator(_TEST_SUITE_SCHEMA).validate(config)
    return config
