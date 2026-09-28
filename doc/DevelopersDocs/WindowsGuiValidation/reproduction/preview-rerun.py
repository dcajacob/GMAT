from pathlib import Path
import json
r=Path('C:/GMAT-Test');a=json.loads((r/'last-jobs.json').read_text());a=[x for x in a if 'ModelPreviewRegression' in x['id']];
for x in a:x['id']='final-preview-'+x['id']
(r/'jobs.json').write_text(json.dumps(a))
