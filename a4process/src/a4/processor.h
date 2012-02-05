#ifndef _A4PROCESS_H_
#define _A4PROCESS_H_

#include <set>

#include <boost/program_options.hpp>

#include <a4/a4io.h>
#include <a4/types.h>
#include <a4/input_stream.h>
#include <a4/output_stream.h>
#include <a4/message.h>
#include <a4/register.h>
#include <a4/object_store.h>

namespace po = ::boost::program_options;

namespace a4{
    namespace process{
        //INTERNAL
        template <class This, typename... TArgs> struct _test_process_as;
        template <class This, class T, class... TArgs> struct _test_process_as<This, T, TArgs...> { 
            static bool process(This* that, const std::string &n, shared<Storable> s) { 
                shared<T> t = dynamic_pointer_cast<T>(s);
                if (t) {
                    that->process(n, t);
                    return true;
                } else return _test_process_as<This, TArgs...>::process(that, n, s);
            }
        };
        template <class This> struct _test_process_as<This> { static bool process(This* that, const std::string &n, shared<Storable> s) { return false; }; };

        using a4::io::A4Message;

        class Driver; 
        class Configuration;
        class OutputAdaptor {
            public:
                virtual void write(shared<const google::protobuf::Message> m) = 0;
                virtual void metadata(shared<google::protobuf::Message> m) = 0;
                void write(const google::protobuf::Message & m) {
                    shared<google::protobuf::Message> msg(m.New());
                    msg->CopyFrom(m);
                    write(msg);
                };
                void metadata(const google::protobuf::Message & m) {
                    shared<google::protobuf::Message> msg(m.New());
                    msg->CopyFrom(m);
                    metadata(msg);
                };
        };

        class Processor {
            public:
                enum MetadataBehavior { AUTO, MANUAL_FORWARD, MANUAL_BACKWARD, DROP };
                MetadataBehavior get_metadata_behavior() { return metadata_behavior; }

                Processor() : my_configuration(NULL), metadata_behavior(AUTO), locked(false) {};
                virtual ~Processor() {};

                /// This function is called at the start of a new metadata block
                /// In here you can modify the metadata_message that is written if auto_metadata is true.
                virtual void process_new_metadata() {};

                /// Override this to process raw A4 Messages without type checking
                virtual void process_message(const A4Message) = 0;

                /// This function is called at the end of a metadata block
                virtual void process_end_metadata() {};

                /// Write a metadata message that (manual_metadata_forward ? starts : ends) a metadata block
                /// To use this method you have to disable automatic metadata writing.
                /// You also need to think about if you want to write your metadata before (manual_metadata_forward = true)
                /// or after (manual_metadata_forward = false) the events it refers to.
                void metadata_start_block(shared<const google::protobuf::Message> m) {
                    metadata_start_block(*m);
                }
                void metadata_start_block(const google::protobuf::Message & m) {
                    assert(metadata_behavior == MANUAL_FORWARD); 
                    _output_adaptor->metadata(m); 
                }
                void metadata_end_block(shared<const google::protobuf::Message> m) { 
                    metadata_end_block(*m);
                }
                void metadata_end_block(const google::protobuf::Message & m) {
                    assert(metadata_behavior == MANUAL_BACKWARD); 
                    _output_adaptor->metadata(m); 
                }

                /// Write a message to the output stream
                void write(shared<const google::protobuf::Message> m) { _output_adaptor->write(m); }
                void write(const google::protobuf::Message & m) { _output_adaptor->write(m); }
                
                /// Write a message to the output stream at most once per event
                void skim(const google::protobuf::Message & m) {
                    if (not skim_written) _output_adaptor->write(m);
                    skim_written = true;
                }

                /// Call channel in process_message to rerun with the prefix "channel/<name>/".
                /// In that run this function always returns true.
                bool channel(const char * name) {
                    rerun_channels.insert(name);
                    if (rerun_channels_current == NULL) return false;
                    return strcmp(rerun_channels_current, name) == 0;
                }
                bool in_channel(const char * name) const {
                    if (rerun_channels_current == NULL) return false;
                    return strcmp(rerun_channels_current, name) == 0;
                }
                /// Call systematic in process_message to rerun with the prefix "syst/<name>/".
                /// In that run this function always returns true.
                bool systematic(const char * name) {
                    rerun_systematics.insert(name);
                    if (rerun_systematics_current == NULL) return false;
                    return strcmp(rerun_systematics_current, name) == 0;
                }
                bool in_systematic(const char * name) const {
                    if (rerun_systematics_current == NULL) return false;
                    return strcmp(rerun_systematics_current, name) == 0;
                }

                /// (unimplemented) From now all histograms are also saved with the prefix "channel/<name>/"
                // void now_channel(const char * name);
                /// (unimplemented) From now on, all histos are saved under prefix "syst/<name>/" and scale <scale>
                // void scale_systematic(const char * c, double scale) { throw a4::Fatal("Not Implemented"); return false; };

                /// Access your own Configuration.
                /// WARNING: there is only one configuration per process, and it is shared by thread!
                /// Therefore, it is const. This may not prevent you from doing non-smart things with it.
                /// suggestion: Do a dynamic_cast to a "MyConfig* config" ONCE in your MyProcessor constructor.
                const Configuration* my_configuration;

                template<class T>
                const T* my() { return dynamic_cast<const T*>(my_configuration); }

                /// Set this flag to skip to the next metadata block
                bool skip_to_next_metadata;

            protected:
                /// In this store you can put named objects.
                /// It will be written and cleared at every metadata block boundary.
                ObjectStore S;

                /// This is the currently valid metadata message. If you manipulate it in AUTO mode,
                /// the changes are written out.
                A4Message metadata_message;

                /// Set the behaviour of metadata
                void set_metadata_behavior(MetadataBehavior m) { assert(!locked); metadata_behavior = m; }

                // Here follows internal stuff.
                std::set<const char *> rerun_channels;
                const char * rerun_channels_current;
                std::set<const char *> rerun_systematics;
                const char * rerun_systematics_current;

                OutputAdaptor* _output_adaptor;

                void lock_and_load() { locked = true; };
                bool skim_written;
                friend class a4::process::Driver;
            private:
                bool locked;
                MetadataBehavior metadata_behavior;
        };

        class Configuration {
            public: 
                virtual ~Configuration() {};
                /// Override this to add options to the command line and configuration file
                virtual void add_options(po::options_description_easy_init) {};
                /// Override this to do further processing of the options from the command line or config file
                virtual void read_arguments(po::variables_map &arguments) {};
                virtual void setup_processor(Processor &g) {};
                virtual Processor * new_processor() = 0;
        };

        template<class ProtoMessage, class ProtoMetaData = a4::io::NoProtoClass>
        class ProcessorOf : public Processor {
            public:
                ProcessorOf() {
                    a4::io::RegisterClass<ProtoMessage> _e;
                    a4::io::RegisterClass<ProtoMetaData> _m;
                };

                /// Override this to proces only your requested messages
                virtual void process(const ProtoMessage &) = 0;

                void process_message(const A4Message msg) {
                    if (!msg) throw a4::Fatal("No message!"); // TODO: Should not be fatal
                    ProtoMessage * pmsg = msg.as<ProtoMessage>().get();
                    if (!pmsg) throw a4::Fatal("Unexpected Message type: ", typeid(*msg.message.get()), " (Expected: ", typeid(ProtoMessage), ")");
                    process(*pmsg);
                };

                ProtoMetaData & metadata() {
                    const A4Message msg = metadata_message;
                    if (!msg) throw a4::Fatal("No metadata at this time!"); // TODO: Should not be fatal
                    ProtoMetaData * meta = msg.as<ProtoMetaData>().get();
                    if (!meta) throw a4::Fatal("Unexpected Metadata type: ", typeid(*msg.message.get()), " (Expected: ", typeid(ProtoMetaData), ")");
                    return *meta;
                };

            protected:
                friend class a4::process::Driver;
        };

        template<class This, class ProtoMetaData = a4::io::NoProtoClass, class... Args>
        class ResultsProcessor : public Processor {
            public:
                ResultsProcessor() { a4::io::RegisterClass<ProtoMetaData> _m; have_name = false; };

                // Generic storable processing
                virtual void process(const std::string &, Storable &) {};

                void process_message(const A4Message msg) {
                    shared<Storable> next = _next_storable(msg);
                    if (next) {
                        if(!_test_process_as<This, Args...>::process((This*)this, next_name, next)) {
                            process(next_name, *next);
                        }
                    }
                };

                shared<Storable> _next_storable(const A4Message msg);

                ProtoMetaData & metadata() {
                    const A4Message msg = metadata_message;
                    if (!msg) throw a4::Fatal("No metadata at this time!"); // TODO: Should not be fatal
                    ProtoMetaData * meta = msg.as<ProtoMetaData>().get();
                    if (!meta) throw a4::Fatal("Unexpected Metadata type: ", typeid(*msg.message.get()), " (Expected: ", typeid(ProtoMetaData), ")");
                    return *meta;
                };
            protected:
                std::string next_name;
                bool have_name;
                friend class a4::process::Driver;
        };


        template<class MyProcessor>
        class ConfigurationOf : public Configuration {
            public:
                /// Override this to setup your thread-safe Processor!
                virtual void setup_processor(MyProcessor &g) {};

                virtual void setup_processor(Processor &g) { setup_processor(dynamic_cast<MyProcessor&>(g)); };
                virtual Processor * new_processor() { Processor * p = new MyProcessor(); p->my_configuration = this; return p;};
        };
    };
};

#endif
