import json,sys,re,subprocess
from pathlib import Path
payloads=[json.loads(line) for line in Path(sys.argv[1]).read_text().splitlines()]
assert len(payloads)==23
ids=set()
for config in payloads[:-1]:
 assert config['unique_id'] not in ids;ids.add(config['unique_id'])
 assert config['device']['identifiers']==['ledclock-123456789abc']
 assert config['device']['name']=='Nerd-Clock'
 assert config['device']['sw_version']=='1.1.1'
 assert config['availability_topic']=='nerd-clock/availability'
 assert len(json.dumps(config).encode())<1900
 if 'command_topic' in config:assert config['retain'] is False
 if '/timer' in config.get('command_topic','') and 'min' in config:assert config['command_template']=='{{ value | int }}'
 if 'value_template' in config:assert config['value_template'].startswith('{{ ') and config['value_template'].endswith(' }}')
 if 'event_types' in config:assert config['event_types']==['timer_finished']
assert payloads[-1]['alarm_active'] is False
for config in payloads[:-1]:
 if 'value_template' in config:
  fields=re.findall(r'value_json\.(\w+)',config['value_template'])
  assert fields and all(field in payloads[-1] for field in fields)
ui=Path(sys.argv[2]).read_text()
script=ui.split('<script>')[1].split('</script>')[0]
# Node checks browser script syntax without needing a browser or dependencies.
subprocess.run(['node','--check'],input=script,text=True,check=True)
print('Behavior, discovery JSON and UI syntax checks passed.')
