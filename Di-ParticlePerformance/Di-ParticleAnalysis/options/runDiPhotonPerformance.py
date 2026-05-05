#!/usr/bin/env python3

from Gaudi.Configuration import INFO
from Gaudi.Configuration import DEBUG
from k4FWCore import ApplicationMgr, IOSvc
from k4FWCore.parseArgs import parser

from Configurables import (
    DiPhotonAnalysis,
    EventDataSvc,
    #THistSvc,
    RootHistSvc,
    AuditorSvc,
    AlgTimingAuditor,
)

from Configurables import Gaudi__Histograming__Sink__Root as RootHistoSink

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
#ILDDiPhoton.HistPath = "/PLOTS"

# Use Gaudi Auditor service to get algorithm timing information
auditorSvc = AuditorSvc()
auditorSvc.Auditors = [AlgTimingAuditor()]

## Histogram output (THistSvc)
#THistSvc().Output = [f"PLOTS DATAFILE='{parsed_args.outputBasename}.root' OPT='RECREATE' TYP='ROOT'"]
hps = RootHistSvc("HistogramPersistencySvc")
root_hist_svc = RootHistoSink("RootHistoSink")
root_hist_svc.FileName = "DiPhoton_histograms.root"

# Configure application manager
app_mgr = ApplicationMgr(
        TopAlg=[ILDDiPhoton],
        EvtSel="NONE",
        EvtMax= -1,
        ExtSvc=[EventDataSvc("EventDataSvc"), auditorSvc, iosvc, root_hist_svc], #THistSvc("THistSvc"),iosvc],
        OutputLevel=DEBUG,
)