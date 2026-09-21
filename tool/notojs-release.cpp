#include <git2.h>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

namespace {

std::runtime_error git_failure(std::string const &message)
{
    auto const *error = git_error_last();
    return std::runtime_error(message + ": " + (error && error->message ? error->message : "unknown Git error"));
}

struct GitLibrary
{
    GitLibrary()
    {
        if(git_libgit2_init() < 0) throw git_failure("Cannot initialize libgit2");
    }

    ~GitLibrary()
    {
        git_libgit2_shutdown();
    }
};

void require_clean_repository(git_repository *repo)
{
    git_status_options options = GIT_STATUS_OPTIONS_INIT;
    options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED
        | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS
        | GIT_STATUS_OPT_INCLUDE_UNREADABLE;

    git_status_list *raw = nullptr;
    if(git_status_list_new(&raw, repo, &options) < 0)
        throw git_failure("Cannot check repository status");
    std::unique_ptr<git_status_list, decltype(&git_status_list_free)> status(raw, git_status_list_free);

    auto const count = git_status_list_entrycount(status.get());
    if(count == 0) return;

    std::string message = "Repository contains uncommitted changes. Commit or stash them before releasing:";
    for(std::size_t i = 0; i < count; ++i)
    {
        auto const *entry = git_status_byindex(status.get(), i);
        auto const *delta = entry->index_to_workdir ? entry->index_to_workdir : entry->head_to_index;
        auto const *path = delta->new_file.path ? delta->new_file.path : delta->old_file.path;
        message += "\n  ";
        message += path ? path : "(unknown path)";
    }
    throw std::runtime_error(message);
}

std::string require_release_upstream(git_repository *repo)
{
    git_reference *raw_head = nullptr;
    if(git_repository_head(&raw_head, repo) < 0)
        throw git_failure("Cannot determine the current branch");
    std::unique_ptr<git_reference, decltype(&git_reference_free)> head(raw_head, git_reference_free);
    std::string const requirement = "Current branch must track master of git@bitbucket.org:iapy/notojs.git";
    if(!git_reference_is_branch(head.get()))
        throw std::runtime_error(requirement + "; HEAD is detached");

    git_buf remote_name = GIT_BUF_INIT;
    std::unique_ptr<git_buf, decltype(&git_buf_dispose)> buffer(&remote_name, git_buf_dispose);
    // Inspect upstream configuration even when its remote-tracking ref is missing.
    int result = git_branch_upstream_merge(&remote_name, repo, git_reference_name(head.get()));
    if(result == GIT_ENOTFOUND) throw std::runtime_error(requirement + "; no upstream branch is configured");
    if(result < 0) throw git_failure("Cannot determine the current branch's upstream branch");
    if(std::string(remote_name.ptr) != "refs/heads/master")
        throw std::runtime_error(requirement + "; upstream branch is " + remote_name.ptr);
    result = git_branch_upstream_remote(&remote_name, repo, git_reference_name(head.get()));
    if(result == GIT_ENOTFOUND) throw std::runtime_error(requirement + "; no upstream remote is configured");
    if(result < 0) throw git_failure("Cannot determine the current branch's upstream remote");
    if(std::string(remote_name.ptr) == ".")
        throw std::runtime_error(requirement + "; upstream is a local branch");

    git_remote *raw_remote = nullptr;
    if(git_remote_lookup(&raw_remote, repo, remote_name.ptr) < 0)
        throw git_failure("Cannot inspect the current branch's upstream remote");
    std::unique_ptr<git_remote, decltype(&git_remote_free)> remote(raw_remote, git_remote_free);
    auto const *url = git_remote_url(remote.get());
    if(!url) throw std::runtime_error("The current branch's upstream remote has no fetch URL");

    std::regex const allowed(
        R"((^(https|ssh)://([^/@]+@)?bitbucket\.org(:[0-9]+)?/iapy/notojs(\.git)?/*$)|(^git@bitbucket\.org:iapy/notojs(\.git)?/*$))",
        std::regex::icase);
    if(!std::regex_match(url, allowed))
        throw std::runtime_error(requirement + "; remote '" + remote_name.ptr + "' points to another repository");
    return remote_name.ptr;
}

int run_git(char const *workdir, std::initializer_list<std::string> arguments)
{
    // Avoid a shell, and let Git use the user's hooks, signing and SSH configuration.
    std::vector<std::string> command{"git", "-C", workdir};
    command.insert(command.end(), arguments.begin(), arguments.end());
    std::vector<char *> argv;
    for(auto &argument : command) argv.push_back(argument.data());
    argv.push_back(nullptr);

    pid_t pid;
    int const error = posix_spawnp(&pid, "git", nullptr, nullptr, argv.data(), environ);
    if(error) throw std::system_error(error, std::generic_category(), "Cannot start git");

    int status;
    while(waitpid(pid, &status, 0) < 0)
        if(errno != EINTR) throw std::system_error(errno, std::generic_category(), "Cannot wait for git");
    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
}

struct GitHubBranch
{
    std::string ref;
    std::string remote;
    git_oid tip;
};

GitHubBranch find_github_branch(git_repository *repo)
{
    git_branch_iterator *raw_iterator = nullptr;
    if(git_branch_iterator_new(&raw_iterator, repo, GIT_BRANCH_LOCAL) < 0)
        throw git_failure("Cannot enumerate local branches");
    std::unique_ptr<git_branch_iterator, decltype(&git_branch_iterator_free)> iterator(raw_iterator, git_branch_iterator_free);

    std::regex const github(
        R"((^(https|ssh)://([^/@]+@)?github\.com(:[0-9]+)?/iapy/notojs(\.git)?/*$)|(^git@github\.com:iapy/notojs(\.git)?/*$))",
        std::regex::icase);
    std::vector<GitHubBranch> matches;
    git_reference *raw_branch = nullptr;
    git_branch_t type;
    int result;
    while((result = git_branch_next(&raw_branch, &type, iterator.get())) == 0)
    {
        std::unique_ptr<git_reference, decltype(&git_reference_free)> branch(raw_branch, git_reference_free);
        git_buf upstream = GIT_BUF_INIT;
        std::unique_ptr<git_buf, decltype(&git_buf_dispose)> buffer(&upstream, git_buf_dispose);
        int error = git_branch_upstream_merge(&upstream, repo, git_reference_name(branch.get()));
        if(error == GIT_ENOTFOUND) continue;
        if(error < 0) throw git_failure("Cannot inspect branch upstream");
        if(std::string(upstream.ptr) != "refs/heads/main") continue;
        error = git_branch_upstream_remote(&upstream, repo, git_reference_name(branch.get()));
        if(error == GIT_ENOTFOUND) continue;
        if(error < 0) throw git_failure("Cannot inspect branch remote");
        if(std::string(upstream.ptr) == ".") continue;

        git_remote *raw_remote = nullptr;
        if(git_remote_lookup(&raw_remote, repo, upstream.ptr) < 0)
            throw git_failure("Cannot inspect remote " + std::string(upstream.ptr));
        std::unique_ptr<git_remote, decltype(&git_remote_free)> remote(raw_remote, git_remote_free);
        auto const *url = git_remote_url(remote.get());
        if(!url || !std::regex_match(url, github)) continue;

        int const checked_out = git_branch_is_checked_out(branch.get());
        if(checked_out < 0) throw git_failure("Cannot check whether the GitHub release branch is checked out");
        if(checked_out)
            throw std::runtime_error("GitHub release branch '" + std::string(git_reference_shorthand(branch.get()))
                + "' is checked out in a worktree; check out another branch there before releasing");
        auto const *tip = git_reference_target(branch.get());
        if(!tip) throw std::runtime_error("GitHub release branch must point directly to a commit");
        matches.push_back({git_reference_name(branch.get()), upstream.ptr, *tip});
    }
    if(result != GIT_ITEROVER) throw git_failure("Cannot enumerate local branches");
    if(matches.empty())
        throw std::runtime_error("A local branch must track main of https://github.com/iapy/notojs before releasing");
    if(matches.size() != 1)
        throw std::runtime_error("Multiple local branches track main of https://github.com/iapy/notojs; the release branch is ambiguous");
    return matches.front();
}

void require_new_tag(git_repository *repo, std::string const &ref)
{
    git_oid existing;
    int const result = git_reference_name_to_id(&existing, repo, ref.c_str());
    if(result == 0) throw std::runtime_error("Release tag already exists: " + ref);
    if(result != GIT_ENOTFOUND) throw git_failure("Cannot check release tag " + ref);
}

void publish_github_release(git_repository *repo, GitHubBranch const &branch,
    git_oid const &source, std::string const &version)
{
    auto const tag = "v" + version;
    auto const tag_ref = "refs/tags/" + tag;
    auto const message = "Release " + tag;

    git_commit *raw_source = nullptr;
    if(git_commit_lookup(&raw_source, repo, &source) < 0)
        throw git_failure("Cannot read the commit pushed to Bitbucket");
    std::unique_ptr<git_commit, decltype(&git_commit_free)> source_commit(raw_source, git_commit_free);
    git_commit *raw_parent = nullptr;
    if(git_commit_lookup(&raw_parent, repo, &branch.tip) < 0)
        throw git_failure("Cannot read the previous GitHub release commit");
    std::unique_ptr<git_commit, decltype(&git_commit_free)> parent(raw_parent, git_commit_free);
    git_tree *raw_tree = nullptr;
    if(git_commit_tree(&raw_tree, source_commit.get()) < 0)
        throw git_failure("Cannot read the tree pushed to Bitbucket");
    std::unique_ptr<git_tree, decltype(&git_tree_free)> tree(raw_tree, git_tree_free);

    auto const *committer = git_commit_committer(source_commit.get());
    git_signature *raw_signature = nullptr;
    if(git_signature_now(&raw_signature, committer->name, committer->email) < 0)
        throw git_failure("Cannot create release commit signature");
    std::unique_ptr<git_signature, decltype(&git_signature_free)> signature(raw_signature, git_signature_free);
    git_commit const *parents[] = {parent.get()};
    git_oid snapshot;
    // Reuse the exact source tree, but only the GitHub branch's history.
    if(git_commit_create(&snapshot, repo, nullptr, signature.get(), signature.get(), nullptr,
        message.c_str(), tree.get(), 1, parents) < 0)
        throw git_failure("Cannot create GitHub release snapshot");

    git_transaction *raw_transaction = nullptr;
    if(git_transaction_new(&raw_transaction, repo) < 0)
        throw git_failure("Cannot prepare release references");
    std::unique_ptr<git_transaction, decltype(&git_transaction_free)> transaction(raw_transaction, git_transaction_free);
    if(git_transaction_lock_ref(transaction.get(), branch.ref.c_str()) < 0
        || git_transaction_lock_ref(transaction.get(), tag_ref.c_str()) < 0)
        throw git_failure("Cannot lock release branch and tag");

    git_reference *raw_current = nullptr;
    if(git_reference_lookup(&raw_current, repo, branch.ref.c_str()) < 0)
        throw git_failure("Cannot recheck GitHub release branch");
    std::unique_ptr<git_reference, decltype(&git_reference_free)> current(raw_current, git_reference_free);
    auto const *tip = git_reference_target(current.get());
    if(!tip || !git_oid_equal(tip, &branch.tip))
        throw std::runtime_error("GitHub release branch changed during the version bump; refusing to overwrite it");
    int const checked_out = git_branch_is_checked_out(current.get());
    if(checked_out < 0) throw git_failure("Cannot recheck GitHub release worktrees");
    if(checked_out) throw std::runtime_error("GitHub release branch was checked out during the version bump");
    require_new_tag(repo, tag_ref);

    if(git_transaction_set_target(transaction.get(), branch.ref.c_str(), &snapshot, signature.get(), message.c_str()) < 0
        || git_transaction_set_target(transaction.get(), tag_ref.c_str(), &snapshot, signature.get(), message.c_str()) < 0)
        throw git_failure("Cannot prepare release branch and tag updates");
    if(git_transaction_commit(transaction.get()) < 0)
        throw git_failure("Cannot finish release reference updates; inspect " + branch.ref + " and " + tag_ref
            + " for snapshot " + git_oid_tostr_s(&snapshot) + " before retrying");
    transaction.reset();

    if(run_git(git_repository_workdir(repo), {"push", "--atomic", "--no-force", "--no-follow-tags", "--",
        branch.remote, branch.ref + ":refs/heads/main", tag_ref + ":" + tag_ref}) != 0)
        throw std::runtime_error("GitHub push failed. Local release commit " + std::string(git_oid_tostr_s(&snapshot))
            + " and tag " + tag + " are preserved. Verify the remote state, then retry an atomic push of "
            + branch.ref + ":refs/heads/main and " + tag_ref + ":" + tag_ref + " to remote '" + branch.remote + "'");

    std::cout << "Published " << message << " and tag " << tag << " to " << branch.remote << "/main\n";
}

void release(char const *path, std::size_t component)
{
    GitLibrary git;
    git_repository *raw = nullptr;
    if(git_repository_open_ext(&raw, path, GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) < 0)
        throw git_failure("Cannot open repository " + std::string(path));

    std::unique_ptr<git_repository, decltype(&git_repository_free)> repo(raw, git_repository_free);
    auto const *workdir = git_repository_workdir(repo.get());
    if(!workdir) throw std::runtime_error("Repository must have a working tree");
    require_clean_repository(repo.get());
    if(git_repository_state(repo.get()) != GIT_REPOSITORY_STATE_NONE)
        throw std::runtime_error("Finish the current Git operation before releasing");
    auto const remote = require_release_upstream(repo.get());

    auto const file = std::filesystem::path(workdir) / "app" / "version.cmake";
    std::ifstream input(file, std::ios::binary);
    if(!input) throw std::runtime_error("Cannot read " + file.string());
    std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if(input.bad()) throw std::runtime_error("Error reading " + file.string());
    input.close();

    // Anchor to a command line so commented-out definitions are not updated.
    std::regex const definition(
        R"version((^|\n)[ \t]*add_compile_definitions\s*\(\s*NOTOJS_VERSION\s*=\s*"(([0-9]+)\.([0-9]+)\.([0-9]+))"\s*\))version");
    auto match = std::sregex_iterator(contents.begin(), contents.end(), definition);
    if(match == std::sregex_iterator())
        throw std::runtime_error("Expected NOTOJS_VERSION=\"major.minor.patch\" in " + file.string());
    auto const version = *match;
    if(++match != std::sregex_iterator())
        throw std::runtime_error("Multiple version definitions in " + file.string());

    std::array<unsigned long long, 3> parts;
    try
    {
        for(std::size_t i = 0; i < parts.size(); ++i)
            parts[i] = std::stoull(version[i + 3].str());
    }
    catch(std::out_of_range const &)
    {
        throw std::runtime_error("Version component is too large in " + file.string());
    }
    if(parts[component] == std::numeric_limits<unsigned long long>::max())
        throw std::runtime_error("Version component cannot be incremented in " + file.string());
    ++parts[component];
    for(std::size_t i = component + 1; i < parts.size(); ++i) parts[i] = 0;

    auto const next = std::to_string(parts[0]) + "." + std::to_string(parts[1]) + "." + std::to_string(parts[2]);
    auto const previous = version[2].str();
    auto const github = find_github_branch(repo.get());
    require_new_tag(repo.get(), "refs/tags/v" + next);

    std::cout << "Release v" << next << "? " << std::flush;
    std::string confirmation;
    if(!std::getline(std::cin, confirmation) || confirmation != "yes")
        throw std::runtime_error("Release cancelled: confirmation must be exactly 'yes'");

    contents.replace(version.position(2), version.length(2), next);

    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    if(!output) throw std::runtime_error("Cannot write " + file.string());
    output << contents;
    output.close();
    if(!output) throw std::runtime_error("Error writing " + file.string());

    std::cout << file.string() << ": " << previous << " -> " << next << std::endl;

    if(run_git(workdir, {"commit", "--only", "-m", "Bump version to " + next, "--", "app/version.cmake"}) != 0)
        throw std::runtime_error("Version updated, but commit failed. Inspect the Git error above and the working tree before retrying");

    git_oid source;
    if(git_reference_name_to_id(&source, repo.get(), "HEAD") < 0)
        throw git_failure("Cannot identify the version bump commit");
    auto const source_id = std::string(git_oid_tostr_s(&source));
    if(run_git(workdir, {"push", "--no-force", "--no-follow-tags", "--", remote, source_id + ":refs/heads/master"}) != 0)
        throw std::runtime_error("Version " + next + " was committed locally, but push failed. "
            "Resolve the Git error above, then push HEAD to master on remote '" + remote
            + "' without running the version bump again");

    std::cout << "Committed and pushed version " << next << " to " << remote << "/master" << std::endl;
    try
    {
        publish_github_release(repo.get(), github, source, next);
    }
    catch(std::exception const &error)
    {
        throw std::runtime_error("Bitbucket already contains version " + next
            + "; GitHub publication did not complete: " + error.what()
            + ". Do not run the version bump again to retry this release");
    }
}

} // namespace

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <repository> <patch|minor|major>\n";
        return EXIT_FAILURE;
    }

    std::string const bump = argv[2];
    std::size_t component;
    if(bump == "major") component = 0;
    else if(bump == "minor") component = 1;
    else if(bump == "patch") component = 2;
    else
    {
        std::cerr << "Invalid version bump '" << bump << "': expected patch, minor, or major\n";
        return EXIT_FAILURE;
    }

    try
    {
        release(argv[1], component);
        return EXIT_SUCCESS;
    }
    catch(std::exception const &error)
    {
        std::cerr << "notojs-release: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
