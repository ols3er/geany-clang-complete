
#include <geanycc/cc_plugin.hpp>
#include <geanycc/utils.hpp>

namespace geanycc
{
    namespace util
    {
		std::vector<std::string>
		get_vector_from_keyfile_stringlist(GKeyFile* keyfile,
										   const char* group, const char* key, GError* error)
		{
			std::vector<std::string> value;
			gsize option_num = 0;
			gchar** strs = g_key_file_get_string_list(keyfile, group, key, &option_num, NULL);
			for (gsize i = 0; i < option_num; i++) {
				value.push_back(strs[i]);
			}
			g_strfreev(strs);
			return value;
		}


		void set_keyfile_stringlist_by_vector(GKeyFile* keyfile, const char* group, const char* key,
										  std::vector<std::string>& value)
		{
			int option_num = value.size();
			gchar** strs = (gchar**)g_malloc0(sizeof(gchar*) * (option_num + 1));
			for (size_t i = 0; i < value.size(); i++) {
				strs[i] = g_strdup(value[i].c_str());
			}
			g_key_file_set_string_list(keyfile, group, key, strs, option_num);
			g_strfreev(strs);
		}


		void save_keyfile(GKeyFile* keyfile, const char* path)
		{
			// TODO use smart_ptr if errors occur, happen memory leak
			gchar* dirname = g_path_get_dirname(path);

			gsize data_length;
			gchar* data = g_key_file_to_data(keyfile, &data_length, NULL);

			int err = utils_mkdir(dirname, TRUE);
			if (err != 0) {
				g_critical(_("Failed to create configuration directory \"%s\": %s"), dirname,
					   g_strerror(err));
				return;
			}

			GError* error = NULL;
			if (!g_file_set_contents(path, data, data_length, &error)) {
				g_critical(_("Failed to save configuration file: %s"), error->message);
				g_error_free(error);
				return;
			}
			g_free(data);
			g_free(dirname);
		}
    } // namespace util
} // namespace geanycc
