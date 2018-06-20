#include "clar_libgit2.h"
#include "config/config_helpers.h"

static git_repository *_repo;
static git_config *_config;

#define TEST_URL "http://github.com/libgit2/libgit2.git"

#define cl_git_assert_cannot_create_remote(expected, creation_expr)	\
	do {								\
		git_remote *r = NULL;					\
		int res = ((creation_expr));				\
		cl_git_fail_with(expected, res);			\
		cl_assert_equal_p(r, NULL);				\
	} while (0);

void test_remote_create__initialize(void)
{
	cl_fixture_sandbox("testrepo.git");

	cl_git_pass(git_repository_open(&_repo, "testrepo.git"));

	cl_git_pass(git_repository_config(&_config, _repo));
}

void test_remote_create__cleanup(void)
{
	git_config_free(_config);

	git_repository_free(_repo);
	_repo = NULL;

	cl_fixture_cleanup("testrepo.git");
}

void test_remote_create__manual(void)
{
	git_remote *remote;
	cl_git_pass(git_config_set_string(_config, "remote.origin.fetch", "+refs/heads/*:refs/remotes/origin/*"));
	cl_git_pass(git_config_set_string(_config, "remote.origin.url", TEST_URL));

	cl_git_pass(git_remote_lookup(&remote, _repo, "origin"));
	cl_assert_equal_s(git_remote_name(remote), "origin");
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);

	git_remote_free(remote);
}

void test_remote_create__named(void)
{
	git_remote *remote;
	git_config *cfg;
	const char *cfg_val;

	cl_skip(); // Later

	size_t section_count = count_config_entries_match(_repo, "remote\\.");

	cl_git_pass(git_remote_create(&remote, _repo, "valid-name", TEST_URL));

	cl_assert_equal_s(git_remote_name(remote), "valid-name");
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_repository_config_snapshot(&cfg, _repo));

	cl_git_pass(git_config_get_string(&cfg_val, cfg, "remote.valid-name.fetch"));
	cl_assert_equal_s(cfg_val, "+refs/heads/*:refs/remotes/valid-name/*");

	cl_git_pass(git_config_get_string(&cfg_val, cfg, "remote.valid-name.url"));
	cl_assert_equal_s(cfg_val, TEST_URL);

	cl_assert_equal_i(section_count + 2, count_config_entries_match(_repo, "remote\\."));

	git_config_free(cfg);
	git_remote_free(remote);
}

void test_remote_create__named_fail_on_invalid_name(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, NULL, TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, "Inv@{id", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, "", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, "/", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, "//", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, ".lock", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create(&r, _repo, "a.lock", TEST_URL));
}

void test_remote_create__named_fail_on_invalid_url(void)
{
	cl_git_assert_cannot_create_remote(GIT_ERROR, git_remote_create(&r, _repo, "bad-url", ""));
}

void test_remote_create__named_fail_on_conflicting_name(void)
{
	cl_git_assert_cannot_create_remote(GIT_EEXISTS, git_remote_create(&r, _repo, "test", TEST_URL));
}

void test_remote_create__with_fetchspec(void)
{
	git_remote *remote;
	git_strarray array;
	size_t section_count = count_config_entries_match(_repo, "remote\\.");

	cl_git_pass(git_remote_create_with_fetchspec(&remote, _repo, "test-new", "git://github.com/libgit2/libgit2", "+refs/*:refs/*"));
	cl_assert_equal_s(git_remote_name(remote), "test-new");
	cl_assert_equal_s(git_remote_url(remote), "git://github.com/libgit2/libgit2");
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_s("+refs/*:refs/*", array.strings[0]);
	cl_assert_equal_i(1, array.count);
	cl_assert_equal_i(section_count + 2, count_config_entries_match(_repo, "remote\\."));

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__with_empty_fetchspec(void)
{
	git_remote *remote;
	git_strarray array;
	size_t section_count = count_config_entries_match(_repo, "remote\\.");

	cl_git_pass(git_remote_create_with_fetchspec(&remote, _repo, "test-new", "git://github.com/libgit2/libgit2", NULL));
	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);
	cl_assert_equal_i(section_count + 1, count_config_entries_match(_repo, "remote\\."));

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__with_fetchspec_invalid_name(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create_with_fetchspec(&r, _repo, NULL, TEST_URL, NULL));
}

void test_remote_create__with_fetchspec_invalid_url(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create_with_fetchspec(&r, _repo, NULL, "", NULL));
}

void test_remote_create__anonymous(void)
{
	git_remote *remote;
	git_strarray array;
	size_t section_count = count_config_entries_match(_repo, "remote\\.");

	cl_git_pass(git_remote_create_anonymous(&remote, _repo, TEST_URL));
	cl_assert_equal_s(git_remote_name(remote), NULL);
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);
	cl_assert_equal_i(section_count, count_config_entries_match(_repo, "remote\\."));

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__anonymous_invalid_url(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create_anonymous(&r, _repo, ""));
}

void test_remote_create__detached(void)
{
	git_remote *remote;
	git_strarray array;

	size_t section_count = count_config_entries_match(_repo, "remote\\.");

	cl_git_pass(git_remote_create_detached(&remote, TEST_URL));
	cl_assert_equal_s(git_remote_name(remote), NULL);
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), NULL);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);
	cl_assert_equal_i(section_count, count_config_entries_match(_repo, "remote\\."));

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__detached_invalid_url(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, git_remote_create_detached(&r, ""));
}

void test_remote_create__with_opts_named(void)
{
	git_remote *remote;
	git_strarray array;
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	opts.name = "test-new";
	opts.repository = _repo;

	cl_skip(); // Later

	cl_git_pass(git_remote_create_with_opts(&remote, TEST_URL, &opts));
	cl_assert_equal_s(git_remote_name(remote), "test-new");
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(1, array.count);
	cl_assert_equal_s("+refs/heads/*:refs/remotes/test-new/*", array.strings[0]);

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__with_opts_named_and_fetchspec(void)
{
	git_remote *remote;
	git_strarray array;
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	opts.name = "test-new";
	opts.repository = _repo;
	opts.fetchspec = "+refs/*:refs/*";

	cl_git_pass(git_remote_create_with_opts(&remote, TEST_URL, &opts));
	cl_assert_equal_s(git_remote_name(remote), "test-new");
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(1, array.count);
	cl_assert_equal_s("+refs/*:refs/*", array.strings[0]);

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__with_opts_named_no_fetchspec(void)
{
	git_remote *remote;
	git_strarray array;
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	opts.name = "test-new";
	opts.repository = _repo;
//	opts.flags = GIT_REMOTE_CREATE_SKIP_DEFAULT_FETCHSPEC; // Later

	cl_git_pass(git_remote_create_with_opts(&remote, TEST_URL, &opts));
	cl_assert_equal_s(git_remote_name(remote), "test-new");
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__with_opts_anonymous(void)
{
	git_remote *remote;
	git_strarray array;
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	opts.repository = _repo;

	cl_git_pass(git_remote_create_with_opts(&remote, TEST_URL, &opts));
	cl_assert_equal_s(git_remote_name(remote), NULL);
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), _repo);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);

	git_strarray_free(&array);
	git_remote_free(remote);
}

void test_remote_create__with_opts_detached(void)
{
	git_remote *remote;
	git_strarray array;
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	cl_git_pass(git_remote_create_with_opts(&remote, TEST_URL, &opts));
	cl_assert_equal_s(git_remote_name(remote), NULL);
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), NULL);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);

	git_strarray_free(&array);

	git_remote_free(remote);

	cl_git_pass(git_remote_create_with_opts(&remote, TEST_URL, NULL));
	cl_assert_equal_s(git_remote_name(remote), NULL);
	cl_assert_equal_s(git_remote_url(remote), TEST_URL);
	cl_assert_equal_p(git_remote_owner(remote), NULL);

	cl_git_pass(git_remote_get_fetch_refspecs(&array, remote));
	cl_assert_equal_i(0, array.count);

	git_strarray_free(&array);

	git_remote_free(remote);
}


void test_remote_create__with_opts_insteadof_disabled(void)
{
	git_remote *remote;
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	opts.repository = _repo;
	opts.flags = GIT_REMOTE_CREATE_SKIP_INSTEADOF;

	cl_git_pass(git_remote_create_with_opts(&remote, "http://example.com/libgit2/libgit2", &opts));

	cl_assert_equal_s(git_remote_url(remote), "http://example.com/libgit2/libgit2");
	cl_assert_equal_p(git_remote_pushurl(remote), NULL);

	git_remote_free(remote);
}

static int create_with_name(git_remote **remote, git_repository *repo, const char *name, const char *url)
{
	git_remote_create_options opts = GIT_REMOTE_CREATE_OPTIONS_INIT;

	opts.repository = repo;
	opts.name = name;

	return git_remote_create_with_opts(remote, url, &opts);
}

void test_remote_create__with_opts_invalid_name(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, "Inv@{id", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, "", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, "/", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, "//", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, ".lock", TEST_URL));
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, "a.lock", TEST_URL));
}

void test_remote_create__with_opts_conflicting_name(void)
{
	cl_git_assert_cannot_create_remote(GIT_EEXISTS, create_with_name(&r, _repo, "test", TEST_URL));
}

void test_remote_create__with_opts_invalid_url(void)
{
	cl_git_assert_cannot_create_remote(GIT_EINVALIDSPEC, create_with_name(&r, _repo, "test-new", ""));
}
