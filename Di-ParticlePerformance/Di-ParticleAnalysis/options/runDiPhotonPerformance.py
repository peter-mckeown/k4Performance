#!/usr/bin/env python3

from Gaudi.Configuration import INFO
from Gaudi.Configuration import DEBUG
from k4FWCore import ApplicationMgr, IOSvc
from k4FWCore.parseArgs import parser

from Configurables import (
    DiPhotonAnalysis,
    EventDataSvc,
    THistSvc,
    AuditorSvc,
    AlgTimingAuditor,
)

parser_group = parser.add_argument_group("runDiPhotonPerformance.py custom options")
parser_group.add_argument("-in","--inputFiles", action="extend", nargs="+", metavar=("file1", "file2"), help="One or multiple input files")
parser_group.add_argument("-out","--outputBasename", help="Basename of the output file(s)", default="DiPhoton_analysis_output")
parsed_args = parser.parse_known_args()[0]

iosvc = IOSvc()
iosvc.Input = parsed_args.inputFiles

iosvc.CollectionNames = ["PandoraPFOs", "MCParticles", "PandoraClusters", "RecoMCTruthLink"]
ILDDiPhoton = DiPhotonAnalysis("ILDDiPhotons")
ILDDiPhoton.OutputLevel = DEBUG
ILDDiPhoton.InputPFOs = ["PandoraPFOs"]
ILDDiPhoton.InputMCParticles = ["MCParticles"]
ILDDiPhoton.InputClusters = ["PandoraClusters"]
ILDDiPhoton.InputRecoMC   = ["RecoMCTruthLink"]
ILDDiPhoton.HistPath = "/PLOTS"

# Use Gaudi Auditor service to get algorithm timing information
auditorSvc = AuditorSvc()
auditorSvc.Auditors = [AlgTimingAuditor()]

## Histogram output (THistSvc)
THistSvc().Output = [f"PLOTS DATAFILE='{parsed_args.outputBasename}.root' OPT='RECREATE' TYP='ROOT'"]

# Configure application manager
app_mgr = ApplicationMgr(
        TopAlg=[ILDDiPhoton],
        EvtSel="None",
        EvtMax=-1,
        ExtSvc=[EventDataSvc("EventDataSvc"), auditorSvc, THistSvc("THistSvc"),iosvc],
        OutputLevel=DEBUG,
)