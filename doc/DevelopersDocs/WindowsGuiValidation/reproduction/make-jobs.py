from pathlib import Path
import sys,json,os,shutil,re
root=Path('C:/GMAT-Test');phase=sys.argv[1];jobs=[]
source=root/'patched/src/UnitTests/TestLinuxGui'
def add(v,name,binary,env=None,fixture=None,timeout=120):
 app=root/v/'application';case=root/'results'/f'{phase}-{v}-{name}';case.mkdir(parents=True,exist_ok=True)
 base=root/'cases'/v
 text=(base/'startup.txt').read_text().replace(base.as_posix()+'/',case.as_posix()+'/')
 (case/'startup.txt').write_text(text);(case/'personalization.ini').write_text('[Main]\nShowWelcomeOnStart=false\n')
 if fixture:fixture(case)
 variables={'GMAT_GUI_TEST_IMAGES':case.as_posix(),'PATH':str(app/'bin')+';'+str(app/'plugins')+';C:\\Python312;'+os.environ['PATH']}
 variables.update(env or {})
 jobs.append({'id':case.name,'command':[str(app/'bin'/f'{binary}.exe'),'--startup_file',str(case/'startup.txt'),'--no_splash'],'cwd':str(app/'bin'),'env':variables,'timeout':timeout})
smoke=(source/'GuiSmoke.script').read_text()
def animation(capacity,trail):
 return lambda c:(c/'animation.script').write_text(smoke.replace('ElapsedSecs = 60','ElapsedSecs = 1500').replace('BeginMissionSequence;',f'BuildTestView.MaxPlotPoints = {capacity};\nBuildTestView.NumPointsToRedraw = {trail};\nBuildTestProp.InitialStepSize = 10;\nBuildTestProp.MaxStep = 10;\nBeginMissionSequence;'))
def groundanimation(c):
 (c/'animation.script').write_text(smoke.replace('ElapsedSecs = 60','ElapsedSecs = 5000').replace('BeginMissionSequence;','Create GroundTrackPlot ReplayTrack;\nReplayTrack.Add = {BuildTestSat};\nBuildTestProp.InitialStepSize = 10;\nBuildTestProp.MaxStep = 10;\nBeginMissionSequence;'))
def deletion(c):
 (c/'deletion.script').write_text(smoke.replace('BeginMissionSequence;','''Create Variable UnusedValue SolveValue;
Create ChemicalTank UsedTank;
BuildTestSat.Tanks = {UsedTank};
Create DifferentialCorrector UsedSolver;
BeginMissionSequence;
Target UsedSolver;
Vary UsedSolver(SolveValue = 1, {Perturbation = 0.1, Lower = 0, Upper = 2, MaxStep = 0.2});
Achieve UsedSolver(SolveValue = 1, {Tolerance = 0.001});
EndTarget;'''))
def python(c):
 (c/'gmat_python_gui.py').write_text('''class Unprintable(Exception):
 def __str__(self): raise RuntimeError('formatting failed')
def valid(): return 7.0
def error(): raise ValueError('bad café: 100% complete')
def unprintable(): raise Unprintable()
def empty(): return []
def ragged(): return [[1.0, 2.0], [3.0]]
def nonnumeric(): return [1.0, 'bad']
def multiple(): return 7.0, [1, 2.5, 3], 'café 100%'
''',encoding='utf-8')
 for name in ['missing-module','missing-function','valid','error','unprintable','empty','ragged','nonnumeric','multiple']:
  fn='Python.gmat_python_gui.'+name
  if name=='missing-module':fn='Python.gmat_no_such_module.valid'
  if name=='missing-function':fn='Python.gmat_python_gui.missing'
  outputs='[result, vec, text]' if name=='multiple' else 'result'
  (c/(name+'.script')).write_text('Create Variable result;\nCreate Array vec[1,3];\nCreate String text;\nBeginMissionSequence;\n'+outputs+' = '+fn+'();\n')
 with (c/'startup.txt').open('a') as f:f.write('\nPYTHON_MODULE_PATH = '+c.as_posix()+'\n')
for v in ['upstream','patched']:
 if phase=='smoke':
  for test in ['Close','Viewport']:add(v,test,test+'Regression')
 elif phase=='focused':
  for test in ['GroundTrack','EditorIo','GroundTrackOwnership','GroundTrackLookup','GroundTrackStation','GroundTrackEditor','TreeTheme','GroundTrackClose','WindowsGroundTrack']:
   add(v,test,test+'Regression')
  add(v,'ResourceDelete','ResourceDeleteRegression',fixture=deletion)
  add(v,'PythonRecovery','PythonGuiRegression',fixture=python)
  add(v,'PythonIOD','PythonIODRegression',{'GMAT_IOD_SCRIPT':str(root/v/'application/samples/Ex_IOD.script')})
  add(v,'WindowsTexture','WindowsTextureRegression',fixture=lambda c:shutil.copy2(root/v/'application/data/vehicle/models/aura.3ds',c/'source.3ds'))
  for mode in ['centered','free','astronaut','shift']:add(v,'Wheel-'+mode,'NativeWheelRegression',{'GMAT_WHEEL_MODE':mode})
  for name,capacity,trail in [('wrapped',31,0),('wrapped-trail',31,5),('unwrapped',256,0),('unwrapped-trail',256,5),('full',151,0),('single',1,0),('wide-trail',31,100)]:add(v,'Animation-'+name,'NativeAnimationRegression',fixture=animation(capacity,trail))
  if v=='patched':add(v,'GroundTrackAnimation','GroundTrackAnimationRegression',fixture=groundanimation)
(root/'jobs.json').write_text(json.dumps(jobs,indent=2));print('Prepared',len(jobs),'jobs')
