#!/usr/bin/env python3
"""Check resource menu access and safe deletion with an unrelated dirty editor."""
from support import context, passed

if __name__ == '__main__':
    ctx = context('resource-delete')
    ctx.runtime()
    script=(ctx.source/'GuiSmoke.script').read_text().replace('BeginMissionSequence;', '''Create Variable UnusedValue SolveValue;
    Create ChemicalTank UsedTank;
    BuildTestSat.Tanks = {UsedTank};
    Create DifferentialCorrector UsedSolver;
    BeginMissionSequence;
    Target UsedSolver;
    Vary UsedSolver(SolveValue = 1, {Perturbation = 0.1, Lower = 0, Upper = 2, MaxStep = 0.2});
    Achieve UsedSolver(SolveValue = 1, {Tolerance = 0.001});
    EndTarget;''')
    (ctx.work/'deletion.script').write_text(script)
    binary = ctx.build_gui('ResourceDeleteRegression.cpp')
    for theme in ('Adwaita', 'Adwaita:dark'):
        ctx.gui(binary, theme.replace(':','-'), environment={**ctx.environment,'GTK_THEME':theme})
        passed('resource deletion: '+theme)
