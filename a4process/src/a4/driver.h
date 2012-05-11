#ifndef _A4_DRIVER_H_
#define _A4_DRIVER_H_

#include <a4/processor.h>

namespace a4 {
namespace process {


class Driver {
    public:
        void set_store(Processor* p, ObjectStore S) {
            p->S = S;
        }
        
    protected:
        void set_metadata(Processor* p, shared<const A4Message> md) {
            p->metadata_message = md;
        }

        void set_output_adaptor(Processor* p, OutputAdaptor* oa) {
            p->_output_adaptor = oa;
        }

        bool try_reading(Processor* p, a4::io::InputStream & istream) {
            return p->try_reading(istream);
        }

        void set_current_message(Processor* p, shared<const A4Message> msg) {
            p->set_current_message(msg);
        }

        static void process_rerun_channels(Processor* p) {
            ObjectStore S = p->S;
            p->process_current_message();
            std::set<const char*> finished_channels;
            do {
                std::set<const char*> channels = p->rerun_channels;
                foreach (const char* c, channels) {
                    if (finished_channels.find(c) != finished_channels.end())
                        continue;
                    p->rerun_channels_current = c;
                    p->S = S("channel/", c, "/");
                    p->process_current_message();
                }
                finished_channels.swap(channels);
            } while (finished_channels.size() != p->rerun_channels.size());
            
            p->S = S;
            p->rerun_channels_current = NULL;
            p->rerun_channels.clear();
        }

        static void process_rerun_systematics(Processor* p) {
            p->skim_written = false;
            ObjectStore S = p->S;
            process_rerun_channels(p);
            
            foreach (const char* c, p->rerun_systematics) {
                p->rerun_systematics_current = c;
                p->S = S("systematic/", c, "/");
                process_rerun_channels(p);
            }
            p->S = S;
            p->rerun_systematics_current = NULL;
            p->rerun_systematics.clear();
        }
};


}
}

#endif
