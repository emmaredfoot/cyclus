// xml_file_loader.cc
// Implements file reader for an XML format
#include "xml_file_loader.h"

#include <fstream>
#include <streambuf>

#include <boost/filesystem.hpp>

#include "blob.h"
#include "context.h"
#include "env.h"
#include "error.h"
#include "model.h"
#include "recipe.h"
#include "timer.h"
#include "xml_query_engine.h"

namespace cyclus {
namespace fs = boost::filesystem;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
XMLFileLoader::XMLFileLoader(Context* ctx,
                             const std::string load_filename) : ctx_(ctx) {
  file_ = load_filename;
  initialize_module_paths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::Init(bool use_main_schema)  {
  std::stringstream input("");
  LoadStringstreamFromFile(input, file_);
  parser_ = boost::shared_ptr<XMLParser>(new XMLParser());
  parser_->Init(input);
  if (use_main_schema) {
    std::stringstream ss(BuildSchema());
    parser_->Validate(ss);
  }

  ctx_->NewEvent("InputFiles")
  ->AddVal("Data", cyclus::Blob(input.str()))
  ->Record();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string XMLFileLoader::BuildSchema() {
  std::stringstream schema("");
  LoadStringstreamFromFile(schema, PathToMainSchema());
  std::string master = schema.str();

  std::set<std::string> types = Model::dynamic_module_types();

  std::stringstream includes;
  std::set<std::string>::iterator type;
  for (type = types.begin(); type != types.end(); ++type) {
    // find modules
    std::stringstream refs;
    refs << std::endl;
    fs::path models_path = Env::GetInstallPath() + "/lib/Models/" + *type;
    fs::recursive_directory_iterator end;
    try {
      for (fs::recursive_directory_iterator it(models_path); it != end; ++it) {
        fs::path p = it->path();
        // build refs and includes
        if (p.extension() == ".rng") {
          includes << "<include href='" << p.string() << "'/>" << std::endl;

          std::ifstream in(p.string().c_str(), std::ios::in | std::ios::binary);
          std::string rng_data((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
          std::string find_str("<define name=\"");
          size_t start = rng_data.find(find_str) + find_str.size();
          size_t end = rng_data.find("\"", start + 1);
          std::string ref = rng_data.substr(start, end - start);
          refs << "<ref name='" << ref << "'/>" << std::endl;
        }
      }
    } catch (...) { }

    // replace refs
    std::string searchStr = std::string("@") + *type + std::string("_REFS@");
    size_t pos = master.find(searchStr);
    if (pos != std::string::npos) {
      master.replace(pos, searchStr.size(), refs.str());
    }
  }

  // replace includes
  std::string searchStr = "@RNG_INCLUDES@";
  size_t pos = master.find(searchStr);
  if (pos != std::string::npos) {
    master.replace(pos, searchStr.size(), includes.str());
  }

  return master;
}

// - - - - - - - - - - - - - - - -   - - - - - - - - - - - - - - - - - -
void XMLFileLoader::LoadStringstreamFromFile(std::stringstream& stream,
                                             std::string file) {

  CLOG(LEV_DEBUG4) << "loading the file: " << file;

  std::ifstream file_stream(file.c_str());

  if (file_stream) {
    stream << file_stream.rdbuf();
    file_stream.close();
  } else {
    throw IOError("The file '" + file
                  + "' could not be loaded.");
  }

  CLOG(LEV_DEBUG5) << "file loaded as a string: " << stream.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string XMLFileLoader::PathToMainSchema() {
  return Env::GetInstallPath() + "/share/cyclus.rng.in";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::ApplySchema(const std::stringstream& schema) {
  parser_->Validate(schema);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::initialize_module_paths() {
  module_paths_["Market"] = "/*/market";
  module_paths_["Converter"] = "/*/converter";
  module_paths_["Region"] = "/*/region";
  module_paths_["Inst"] = "/*/region/institution";
  module_paths_["Facility"] = "/*/facility";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::load_recipes() {
  XMLQueryEngine xqe(*parser_);

  std::string query = "/*/recipe";
  int numRecipes = xqe.NElementsMatchingQuery(query);
  for (int i = 0; i < numRecipes; i++) {
    QueryEngine* qe = xqe.QueryElement(query, i);
    recipe::Load(ctx_, qe);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::load_dynamic_modules(
  std::set<std::string>& module_types) {
  std::set<std::string>::iterator it;
  for (it = module_types.begin(); it != module_types.end(); it++) {
    load_modules_of_type(*it, module_paths_[*it]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::load_modules_of_type(std::string type,
                                         std::string query_path) {
  XMLQueryEngine xqe(*parser_);

  int numModels = xqe.NElementsMatchingQuery(query_path);
  for (int i = 0; i < numModels; i++) {
    QueryEngine* qe = xqe.QueryElement(query_path, i);
    Model::InitializeSimulationEntity(ctx_, type, qe);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void XMLFileLoader::load_control_parameters(Timer* ti) {
  XMLQueryEngine xqe(*parser_);

  std::string query = "/*/control";
  QueryEngine* qe = xqe.QueryElement(query);
  ti->load_simulation(ctx_, qe);
}
} // namespace cyclus