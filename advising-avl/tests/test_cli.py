"""Compare real processes; preserve original workflow for supported inputs."""
import subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def run(binary, commands):
 p=subprocess.run([str(root/'build'/binary)],input=commands,text=True,capture_output=True,cwd=root,timeout=20)
 assert p.returncode==0, p.stderr
 return p.stdout
with tempfile.TemporaryDirectory() as temp:
 p=Path(temp)
 (p/'good.csv').write_text('A,Alpha\nB,Beta,A\n')
 (p/'bad.csv').write_text('X,Broken,MISSING\n')
 scenarios = {
 'course list and prerequisites':'1\noriginal/ABCU_Advising_Program_Input.csv\n2\n3\n csci400 \n9\n',
 'empty state menu and EOF':'2\n3\nxyz\n\n',
 'missing file':'1\nnot-found.csv\n9\n',
 'rejected replacement':f'1\n{p}/good.csv\n1\n{p}/bad.csv\n2\n3\nB\n9\n',
 'valid replacement':f'1\noriginal/ABCU_Advising_Program_Input.csv\n1\n{p}/good.csv\n2\n9\n',
 'quoted path and missing course':f'1\n"{p}/good.csv"\n3\nZ\n3\n\n9\n',
 'EOF during load':'1\n',
 }
 for name,commands in scenarios.items():
  before,after=run('original',commands),run('advising',commands)
  assert before==after, name
  print('PASS CLI equivalence:',name)
 print('RESULT: 7 original/enhanced CLI scenarios matched exactly.')
 transcript=run('advising',scenarios['rejected replacement'])
 (root/'evidence/cli-rejected-replacement.txt').write_text(transcript.replace(str(p),'<temporary fixture directory>'))
