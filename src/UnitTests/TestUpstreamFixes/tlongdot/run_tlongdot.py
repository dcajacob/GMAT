#!/usr/bin/env python3
"""Portable exact-source TLONGDot regression; Python stdlib + git + C++17 only."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

SOURCE_PATH = "src/base/parameter/OrbitData.cpp"
REFERENCE_REVISION = "9363e129be366520c6edb0b4079204ed60666007"
VANILLA_BASE_REVISION = "c9b78bd1c8499845cb78fde315e1f761b6b06f1c"
REFERENCE_FUNCTION_SHA256 = "8b6411d97af0c11e5ae13c293bd529ea855780e45bfe1c8fedcd4d6d55a386ce"
MU = 398600.4418  # km^3/s^2; test input, not an imported ephemeris constant

# Historical NASA executions, not newly generated independent truth. The source
# file is unchanged between REFERENCE_REVISION and VANILLA_BASE_REVISION. These
# six-component values protect the already-existing nonzero-perturbation branch.
# They were generated with GCC 15.2.0, IEEE binary64, exact-source function stubs.
PERTURBED_REFERENCE = {
    "radial_perturbation": [0.0017078083853383137, 1.4943323371717839e-7,
        5.4389249083277025e-8, 8.9834220378076e-21, -2.4681749198307394e-20, 0.04619287660513877],
    "tangential_perturbation": [0.0051651207264086195, 1.5818747485993917e-7,
        -2.8088307061238553e-7, 2.232355117597525e-21, -6.133345277803165e-21, 0.04619287660513877],
    "normal_perturbation": [4.728504124082431e-16, 2.7355170979659365e-20,
        -2.9695057689283765e-8, 3.290518606384564e-8, -9.040625569336878e-8, 0.04619713010883376],
    "retrograde_normal_perturbation": [-3.2466868633887455e-16, -2.0639115080670927e-20,
        -8.393159677166442e-8, 9.568765585799126e-8, -2.6289967379037564e-7, 0.0462048989207958],
    "near_retrograde_normal_perturbation": [9.642178627938039e-16, 7.040216840620509e-20,
        -5.552868335109062e-5, 0.03141241240080756, -0.08630489376181509, 0.054146774599978416],
    "zero_total_acceleration": [8.658250352840303, 0.0007575969058735266,
        0.00027574272331013774, 7.135174843292599e-21, -1.9603731765807078e-20, 0.04619287660513877],
}

# Stubs deliberately replace dependencies only. The production function below
# this support code is extracted verbatim from the requested immutable commit.
SUPPORT = r'''
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
using Real = double;
using Integer = int;
namespace GmatMathConstants {
constexpr double RAD_PER_DEG = 3.14159265358979323846 / 180.0;
constexpr double DEG_PER_RAD = 180.0 / 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * 3.14159265358979323846;
}
namespace Gmat { constexpr int ODE_MODEL = 1; }
struct Rvector3 {
 double d[3]; Rvector3(double a=0, double b=0, double c=0): d{a,b,c} {}
 double& operator[](int n) { return d[n]; }
 const double& operator[](int n) const { return d[n]; }
 double GetMagnitude() const { return std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]); }
 Rvector3 operator/(double k) const { return {d[0]/k,d[1]/k,d[2]/k}; }
};
Rvector3 Cross(const Rvector3&a, const Rvector3&b) {
 return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
struct Rvector6 {
 double d[6];
 Rvector6(double a=0,double b=0,double c=0,double e=0,double f=0,double g=0): d{a,b,c,e,f,g} {}
 double& operator[](int n) { return d[n]; }
 const double& operator[](int n) const { return d[n]; }
 Rvector3 GetV() const { return {d[3],d[4],d[5]}; }
};
using ParameterException = std::runtime_error;
struct ODEModel {
 Rvector6 dv;
 Rvector6 GetDerivativesForSpacecraft(void*) { return dv; }
};
struct Ref { int objType; void*obj; };
struct OrbitData {
 ODEModel *ode; void*mSpacecraft=nullptr; int mNumRefObjects=0; Ref*mRefObjList=nullptr;
 double mGravConst; Rvector6 eq,kep;
 Rvector6 GetEquinState() { return eq; }
 Rvector6 GetKepState() { return kep; }
 Rvector6 GetEquinDot(Rvector3&, Rvector3&, Real);
};
'''
MAIN = r'''
int main() {
 OrbitData o; ODEModel model; o.ode=&model; Rvector3 r,v,pert;
 while (std::cin >> o.mGravConst) {
  for(int i=0;i<3;i++) std::cin >> r[i];
  for(int i=0;i<3;i++) std::cin >> v[i];
  for(int i=0;i<3;i++) std::cin >> pert[i];
  for(int i=0;i<6;i++) std::cin >> o.eq[i];
  for(int i=0;i<6;i++) std::cin >> o.kep[i];
  if(!std::cin) return 2;
  double rm=r.GetMagnitude(), r3=rm*rm*rm;
  // Exact cancellation is intentional: zero perturbations must reach the old
  // zero branch instead of accidentally exercising a floating-point residual.
  for(int i=0;i<3;i++) model.dv[i+3]=-o.mGravConst*r[i]/r3+pert[i];
  auto recovered=model.dv.GetV();
  for(int i=0;i<3;i++) recovered[i]+=o.mGravConst*r[i]/r3;
  auto rate=o.GetEquinDot(r,v,rm);
  std::cout << std::setprecision(17);
  for(int i=0;i<6;i++) std::cout << (i?" ":"") << rate[i];
  std::cout << " " << recovered.GetMagnitude() << "\n";
 }
}
'''


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def extract_function(source):
    """Find balanced C++ braces while ignoring comment/string/character braces."""
    match = re.search(r"Rvector6\s+OrbitData::GetEquinDot\s*\(", source)
    if not match:
        raise ValueError("GetEquinDot definition not found")
    start = match.start()
    i = source.index("{", match.end())
    depth = 0
    state = "code"
    while i < len(source):
        ch = source[i]
        pair = source[i:i+2]
        if state == "line":
            if ch == "\n":
                state = "code"
        elif state == "block":
            if pair == "*/":
                state = "code"
                i += 1
        elif state in ('"', "'"):
            if ch == "\\":
                i += 1
            elif ch == state:
                state = "code"
        elif pair in ("//", "/*"):
            state = "line" if pair == "//" else "block"
            i += 1
        elif ch in ('"', "'"):
            state = ch
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return source[start:i+1]
        i += 1
    raise ValueError("Unbalanced GetEquinDot definition")


def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def norm(a): return math.sqrt(dot(a,a))


def orbital_state(a,e,inc,raan,argp,nu):
    inc,raan,argp,nu=map(math.radians,(inc,raan,argp,nu))
    p=a*(1-e*e)
    u=argp+nu
    co,so,ci,si=math.cos(raan),math.sin(raan),math.cos(inc),math.sin(inc)
    cu,su=math.cos(u),math.sin(u)
    radial=[co*cu-so*su*ci,so*cu+co*su*ci,su*si]
    tangent=[-co*su-so*cu*ci,-so*su+co*cu*ci,cu*si]
    normal=cross(radial,tangent)
    radius=p/(1+e*math.cos(nu))
    r=[radius*x for x in radial]
    vr=math.sqrt(MU/p)*e*math.sin(nu)
    vt=math.sqrt(MU/p)*(1+e*math.cos(nu))
    v=[vr*x+vt*y for x,y in zip(radial,tangent)]
    eq=[a,e*math.sin(raan+argp),e*math.cos(raan+argp),
        math.tan(inc/2)*so,math.tan(inc/2)*co,0]
    kep=[a,e,*map(math.degrees,(inc,raan,argp,nu))]
    analytic=math.degrees(math.sqrt(MU/(p*p*p))*(1+e*math.cos(nu))**2)
    return r,v,radial,tangent,normal,eq,kep,analytic


def longitude(r,v):
    """RAAN+argument of latitude from nodes; no equinoctial rate equations."""
    h=cross(r,v)
    hn=norm(h)
    node=[-h[1],h[0],0]
    nn=norm(node)
    if nn < 1e-12*hn:
        return math.atan2(r[1],r[0])  # only prograde equatorial cases used
    nhat=[x/nn for x in node]
    qhat=cross([x/hn for x in h],nhat)
    return math.atan2(node[1],node[0])+math.atan2(dot(r,qhat),dot(r,nhat))


def finite_difference(r,v,acc,dt):
    rp=[x+y*dt+0.5*z*dt*dt for x,y,z in zip(r,v,acc)]
    rm=[x-y*dt+0.5*z*dt*dt for x,y,z in zip(r,v,acc)]
    vp=[x+y*dt for x,y in zip(v,acc)]
    vm=[x-y*dt for x,y in zip(v,acc)]
    delta=longitude(rp,vp)-longitude(rm,vm)
    return math.degrees(math.atan2(math.sin(delta),math.cos(delta))/(2*dt))


def make_cases():
    definitions=[
        ("circular_equatorial_central",(7000,0,0,0,0,0),(0,0,0)),
        ("eccentric_periapsis_central",(12000,.4,0,0,0,0),(0,0,0)),
        ("eccentric_apoapsis_central",(12000,.4,0,0,0,180),(0,0,0)),
        ("eccentric_inclined_central",(12000,.4,63,40,50,70),(0,0,0)),
        ("eccentric_retrograde_central",(12000,.4,120,40,50,70),(0,0,0)),
        ("longitude_wrap_central",(12000,.4,63,40,250,70),(0,0,0)),
        ("radial_perturbation",(12000,.4,63,40,50,70),(1e-6,0,0)),
        ("tangential_perturbation",(12000,.4,63,40,50,70),(0,1e-6,0)),
        ("normal_perturbation",(12000,.4,63,40,50,70),(0,0,1e-6)),
        ("retrograde_normal_perturbation",(12000,.4,120,40,50,70),(0,0,1e-6)),
        ("near_retrograde_normal_perturbation",(12000,.4,179.9,40,50,70),(0,0,1e-6)),
        ("zero_total_acceleration",(12000,.4,63,40,50,70),None),
    ]
    cases=[]
    for name,els,rtn in definitions:
        r,v,radial,tangent,normal,eq,kep,analytic=orbital_state(*els)
        radius=norm(r)
        perturb=([MU*x/radius**3 for x in r] if rtn is None else
            [sum(c*b[j] for c,b in zip(rtn,(radial,tangent,normal))) for j in range(3)])
        acceleration=[-MU*x/radius**3+y for x,y in zip(r,perturb)]
        cases.append({"name":name,"elements_a_e_i_raan_argp_nu":els,
            "perturbation_rtn_km_s2":rtn,"input":[MU,*r,*v,*perturb,*eq,*kep],
            "position_km":r,"velocity_km_s":v,"acceleration_km_s2":acceleration,
            "central":rtn==(0,0,0),"analytic_two_body_deg_s":analytic,
            "finite_difference_deg_s":{str(dt):finite_difference(r,v,acceleration,dt)
                for dt in (.1,.01,.001)}})
    return cases


def assess(cases, rows):
    checks=[]
    def check(case,contract,passed,**details):
        checks.append({"case":case,"contract":contract,"passed":bool(passed),**details})
    if len(rows)!=len(cases):
        raise ValueError(f"Expected {len(cases)} native output rows, got {len(rows)}")
    for c,row in zip(cases,rows):
        if len(row)!=7 or not all(math.isfinite(x) for x in row):
            raise ValueError(f"Invalid native row for {c['name']}: {row}")
        rates,residual=row[:6],row[6]
        c["native_rates"]=rates
        c["native_recovered_perturbation_norm_km_s2"]=residual
        name=c["name"]
        check(name,"branch_precondition",residual==0 if c["central"] else residual>0,
            residual_norm=residual,expected="zero" if c["central"] else "positive")
        if c["central"]:
            expected=c["analytic_two_body_deg_s"]
            check(name,"nonzero_two_body_TLONGDot",math.isclose(rates[5],expected,rel_tol=2e-12,abs_tol=2e-13),
                actual=rates[5],expected=expected,unit="deg/s")
            check(name,"other_unperturbed_rates_zero",all(x==0 for x in rates[:5]),actual=rates[:5])
        else:
            expected=PERTURBED_REFERENCE[name]
            absolute=[2e-12,2e-16,2e-16,2e-15,2e-15,2e-12]
            check(name,"unchanged_NASA_nonzero_perturbation_vector",
                all(math.isclose(a,b,rel_tol=2e-11,abs_tol=t) for a,b,t in zip(rates,expected,absolute)),
                actual=rates,expected=expected,absolute_tolerance=absolute,relative_tolerance=2e-11)
        for dt,tol in ((.1,2e-9),(.01,1e-10),(.001,1e-10)):
            expected=c["finite_difference_deg_s"][str(dt)]
            check(name,"independent_Cartesian_longitude_finite_difference",
                abs(rates[5]-expected)<=tol,dt_s=dt,actual=rates[5],expected=expected,
                absolute_tolerance_deg_s=tol)
    return checks


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo",required=True,type=Path,help="NASA GMAT Git checkout; read only")
    parser.add_argument("--revision",required=True,help="Commit SHA or ref, resolved once to an immutable commit")
    parser.add_argument("--output",required=True,type=Path,help="New or empty output directory")
    parser.add_argument("--cxx",default="c++",help="C++17 compiler executable (default c++)")
    args=parser.parse_args()
    out=args.output.expanduser().resolve()
    if out.exists() and (not out.is_dir() or any(out.iterdir())):
        parser.error("--output must be a new or empty directory")
    out.mkdir(parents=True,exist_ok=True)
    report={"status":"error","started_utc":datetime.now(timezone.utc).isoformat(),
        "invocation":[sys.executable,str(Path(__file__).resolve()),*sys.argv[1:]],
        "source_path":SOURCE_PATH,"requested_revision":args.revision,
        "reference_provenance":{"revision":REFERENCE_REVISION,"function_sha256":REFERENCE_FUNCTION_SHA256,
            "identical_source_at_vanilla_base":VANILLA_BASE_REVISION,
            "origin":"Historical GCC 15.2.0 exact-source NASA execution; not a new independent oracle"},
        "commands":[],"limitations":[
            "Only the exact extracted GetEquinDot body is production code under test.",
            "Explicit vector, ODEModel, reference-list, exception and element-conversion stubs replace dependencies.",
            "Not a linked GMAT engine, Python binding, coordinate-frame conversion, force-model or propagation integration test.",
            "Finite differences use a local Taylor state expansion and Cartesian node geometry, not a full orbit integration.",
            "The independently checked scalar is true-longitude rate; the other perturbed components use recorded NASA regression values.",
            "Singular radial states, i=180 degrees, unbound conics, extreme scaling and missing-ODE handling are not qualified."]}
    log=[]
    def command(argv,input_text=None,timeout=60,log_stdout=True):
        log.append("$ "+shlex.join(argv))
        p=subprocess.run(argv,input=input_text,text=True,capture_output=True,timeout=timeout)
        saved_stdout=p.stdout if log_stdout else "[source content omitted; exact function saved separately]"
        report["commands"].append({"argv":argv,"returncode":p.returncode,"stdout":saved_stdout,"stderr":p.stderr})
        log.extend([saved_stdout,p.stderr,f"exit={p.returncode}"])
        if p.returncode:
            raise RuntimeError(f"Command failed ({p.returncode}): {shlex.join(argv)}")
        return p.stdout
    exit_code=2
    try:
        repo=args.repo.expanduser().resolve(strict=True)
        report["repository"]=str(repo)
        git=["git","--no-optional-locks","-C",str(repo)]
        revision=command([*git,"rev-parse","--verify","--end-of-options",args.revision+"^{commit}"]).strip()
        if not re.fullmatch(r"[0-9a-f]{40,64}",revision):
            raise ValueError("Git did not return an immutable commit hash")
        report["resolved_revision"]=revision
        source=command([*git,"show",revision+":"+SOURCE_PATH],log_stdout=False)
        report["source_blob"]=command([*git,"rev-parse",revision+":"+SOURCE_PATH]).strip()
        report["source_file_sha256"]=sha256(source.encode())
        function=extract_function(source)
        report["function_sha256"]=sha256(function.encode())
        (out/"GetEquinDot.extracted.cpp").write_text(function+"\n")
        # Preserve upstream copyright/license prologue in generated source.
        prologue=source.split("#include",1)[0]
        cpp=out/"probe.cpp"
        cpp.write_text("// Generated regression fixture. Only GetEquinDot is extracted verbatim.\n"+
            prologue+SUPPORT+"\n"+function+"\n"+MAIN)
        report["generated_source_sha256"]=sha256(cpp.read_bytes())
        cxx=shutil.which(args.cxx)
        if cxx is None:
            raise ValueError(f"C++ compiler not found: {args.cxx}")
        report["compiler_identity"]=command([cxx,"--version"])
        binary=out/"probe"
        command([cxx,"-std=c++17","-O2",str(cpp),"-o",str(binary)],timeout=120)
        report["binary_sha256"]=sha256(binary.read_bytes())
        cases=make_cases()
        input_text="\n".join(" ".join(map(repr,c["input"])) for c in cases)+"\n"
        (out/"inputs.txt").write_text(input_text)
        rows=[list(map(float,line.split())) for line in command([str(binary)],input_text=input_text).splitlines()]
        checks=assess(cases,rows)
        failed=[c for c in checks if not c["passed"]]
        report.update(cases=cases,checks=checks,summary={"cases":len(cases),"checks":len(checks),
            "passed":len(checks)-len(failed),"failed":len(failed)},status="fail" if failed else "pass")
        for item in failed:
            log.append("FAIL "+json.dumps(item,sort_keys=True))
        exit_code=1 if failed else 0
    except (OSError,ValueError,RuntimeError,subprocess.SubprocessError) as error:
        report["error"]=str(error)
        log.append("ERROR "+str(error))
    report["exit_code"]=exit_code
    report["finished_utc"]=datetime.now(timezone.utc).isoformat()
    log.append(json.dumps({"status":report["status"],"exit_code":exit_code,"summary":report.get("summary")}))
    (out/"results.json").write_text(json.dumps(report,indent=2,allow_nan=False)+"\n")
    (out/"run.log").write_text("\n".join(log)+"\n")
    print(f"TLONGDot {report['status']}: {report.get('summary',report.get('error'))}; evidence={out}")
    return exit_code


if __name__=="__main__":
    sys.exit(main())
