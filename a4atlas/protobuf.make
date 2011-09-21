protodir=${localstatedir}/a4/proto/$(A4PACK)
protoincludedir=${includedir}/a4/proto/$(A4PACK)
protopythondir=${pythondir}/a4/proto/$(A4PACK)

PYDIR=$(builddir)/python/a4/proto/$(A4PACK)
CPPDIR=$(builddir)/src/a4/proto/$(A4PACK)
PROTOBUF_CFLAGS += -I$(builddir)/$(CPPDIR)

PROTOBUF_PY=$(PYDIR)/Histograms_pb2.py $(PYDIR)/Photon_pb2.py $(PYDIR)/Cutflow_pb2.py $(PYDIR)/Event_pb2.py $(PYDIR)/Jet_pb2.py $(PYDIR)/Electron_pb2.py $(PYDIR)/Physics_pb2.py $(PYDIR)/Muon_pb2.py $(PYDIR)/Results_pb2.py $(PYDIR)/Atlas/Isolation_pb2.py $(PYDIR)/Atlas/TrackHits_pb2.py $(PYDIR)/Atlas/DataQuality_pb2.py $(PYDIR)/Atlas/Trigger_pb2.py $(PYDIR)/Atlas/ShowerShape_pb2.py $(PYDIR)/Atlas/EventStreamInfo_pb2.py
PROTOBUF_H=$(CPPDIR)/Histograms.pb.h $(CPPDIR)/Photon.pb.h $(CPPDIR)/Cutflow.pb.h $(CPPDIR)/Event.pb.h $(CPPDIR)/Jet.pb.h $(CPPDIR)/Electron.pb.h $(CPPDIR)/Physics.pb.h $(CPPDIR)/Muon.pb.h $(CPPDIR)/Results.pb.h $(CPPDIR)/Atlas/Isolation.pb.h $(CPPDIR)/Atlas/TrackHits.pb.h $(CPPDIR)/Atlas/DataQuality.pb.h $(CPPDIR)/Atlas/Trigger.pb.h $(CPPDIR)/Atlas/ShowerShape.pb.h $(CPPDIR)/Atlas/EventStreamInfo.pb.h
PROTOBUF_CC=$(CPPDIR)/Histograms.pb.cc $(CPPDIR)/Photon.pb.cc $(CPPDIR)/Cutflow.pb.cc $(CPPDIR)/Event.pb.cc $(CPPDIR)/Jet.pb.cc $(CPPDIR)/Electron.pb.cc $(CPPDIR)/Physics.pb.cc $(CPPDIR)/Muon.pb.cc $(CPPDIR)/Results.pb.cc $(CPPDIR)/Atlas/Isolation.pb.cc $(CPPDIR)/Atlas/TrackHits.pb.cc $(CPPDIR)/Atlas/DataQuality.pb.cc $(CPPDIR)/Atlas/Trigger.pb.cc $(CPPDIR)/Atlas/ShowerShape.pb.cc $(CPPDIR)/Atlas/EventStreamInfo.pb.cc
PROTOBUF_PROTO=$(srcdir)/proto/Histograms.proto $(srcdir)/proto/Photon.proto $(srcdir)/proto/Cutflow.proto $(srcdir)/proto/Event.proto $(srcdir)/proto/Jet.proto $(srcdir)/proto/Electron.proto $(srcdir)/proto/Physics.proto $(srcdir)/proto/Muon.proto $(srcdir)/proto/Results.proto $(srcdir)/proto/Atlas/Isolation.proto $(srcdir)/proto/Atlas/TrackHits.proto $(srcdir)/proto/Atlas/DataQuality.proto $(srcdir)/proto/Atlas/Trigger.proto $(srcdir)/proto/Atlas/ShowerShape.proto $(srcdir)/proto/Atlas/EventStreamInfo.proto

CLEANFILES += $(PYDIR)/__init__.py
protoAtlasdir=${protodir}/Atlas
protoincludeAtlasdir=${protoincludedir}/Atlas
protopythonAtlasdir=${protopythondir}/Atlas

$(PYDIR)/Atlas/__init__.py:
	touch $@

CLEANFILES += $(PYDIR)/Atlas/__init__.py

dist_proto_DATA=$(srcdir)/proto/Muon.proto $(srcdir)/proto/Jet.proto $(srcdir)/proto/Histograms.proto $(srcdir)/proto/Photon.proto $(srcdir)/proto/Results.proto $(srcdir)/proto/Cutflow.proto $(srcdir)/proto/Electron.proto $(srcdir)/proto/Physics.proto $(srcdir)/proto/Event.proto
nodist_protoinclude_HEADERS=$(CPPDIR)/Muon.pb.h $(CPPDIR)/Jet.pb.h $(CPPDIR)/Histograms.pb.h $(CPPDIR)/Photon.pb.h $(CPPDIR)/Results.pb.h $(CPPDIR)/Cutflow.pb.h $(CPPDIR)/Electron.pb.h $(CPPDIR)/Physics.pb.h $(CPPDIR)/Event.pb.h
nodist_protopython_PYTHON=$(PYDIR)/Muon_pb2.py $(PYDIR)/Jet_pb2.py $(PYDIR)/Histograms_pb2.py $(PYDIR)/Photon_pb2.py $(PYDIR)/Results_pb2.py $(PYDIR)/Cutflow_pb2.py $(PYDIR)/Electron_pb2.py $(PYDIR)/Physics_pb2.py $(PYDIR)/Event_pb2.py $(PYDIR)/__init__.py

dist_protoAtlas_DATA=$(srcdir)/proto/Atlas/Isolation.proto $(srcdir)/proto/Atlas/ShowerShape.proto $(srcdir)/proto/Atlas/TrackHits.proto $(srcdir)/proto/Atlas/Trigger.proto $(srcdir)/proto/Atlas/EventStreamInfo.proto $(srcdir)/proto/Atlas/DataQuality.proto
nodist_protoincludeAtlas_HEADERS=$(CPPDIR)/Atlas/Isolation.pb.h $(CPPDIR)/Atlas/ShowerShape.pb.h $(CPPDIR)/Atlas/TrackHits.pb.h $(CPPDIR)/Atlas/Trigger.pb.h $(CPPDIR)/Atlas/EventStreamInfo.pb.h $(CPPDIR)/Atlas/DataQuality.pb.h
nodist_protopythonAtlas_PYTHON=$(PYDIR)/Atlas/Isolation_pb2.py $(PYDIR)/Atlas/ShowerShape_pb2.py $(PYDIR)/Atlas/TrackHits_pb2.py $(PYDIR)/Atlas/Trigger_pb2.py $(PYDIR)/Atlas/EventStreamInfo_pb2.py $(PYDIR)/Atlas/DataQuality_pb2.py $(PYDIR)/Atlas/__init__.py


# how to make protobuf objects
$(PYDIR)/%_pb2.py $(CPPDIR)/%.pb.cc $(CPPDIR)/%.pb.h: $(srcdir)/proto/%.proto
	@mkdir -p $(PYDIR)
	@mkdir -p $(CPPDIR)
	${PROTOBUF_PROTOC} -I=$(srcdir)/proto --python_out $(PYDIR) --cpp_out $(CPPDIR) $<

# how to make the python __init__.py
$(PYDIR)/__init__.py: $(PROTOBUF_PY)
	grep -Ho 'class [A-Za-z0-9]*' $^ | sed 's/.py:class/ import/' | sed "s/python\/a4\/proto\/$(A4PACK)\//from ./" | sed 's/\//./g' > $@

# make sure all protobuf are generated before they are built!
BUILT_SOURCES=$(PROTOBUF_H) $(PROTOBUF_CC)

CLEANFILES += $(PROTOBUF_H) $(PROTOBUF_CC) $(PROTOBUF_PY)

