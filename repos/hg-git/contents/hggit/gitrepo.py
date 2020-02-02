import os
from mercurial import util
try:
    from mercurial.error import RepoError
except ImportError:
    from mercurial.repo import RepoError

try:
    from mercurial.peer import peerrepository
except ImportError:
    from mercurial.repo import repository as peerrepository

from git_handler import GitHandler
from dulwich.errors import HangupException, GitProtocolError
from dulwich.protocol import ZERO_SHA
from mercurial.i18n import _
from mercurial import context, util as hgutil

from overlay import overlayrepo

from mercurial.node import bin

class gitrepo(peerrepository):
    capabilities = ['lookup']

    def _capabilities(self):
        return self.capabilities

    def __init__(self, ui, path, create):
        if create: # pragma: no cover
            raise util.Abort('Cannot create a git repository.')
        self.ui = ui
        self.path = path
        self.localrepo = None
        self.handler = None

    def _initializehandler(self):
        if self.handler is None and self.localrepo is not None:
            self.handler = GitHandler(self.localrepo, self.localrepo.ui)
        return self.handler

    def url(self):
        return self.path

    def lookup(self, key):
        if isinstance(key, str):
            return key

    def local(self):
        if not self.path:
            raise RepoError

    def heads(self):
        return []

    def listkeys(self, namespace):
        if namespace == 'namespaces':
            return {'bookmarks':''}
        elif namespace == 'bookmarks':
            handler = self._initializehandler()
            if handler:
                handler.export_commits()
                refs = handler.fetch_pack(self.path)
                reqrefs = refs
                convertlist, commits = handler.getnewgitcommits(reqrefs)
                newcommits = [bin(c) for c in commits]
                b = overlayrepo(handler, newcommits, refs)
                stripped_refs = dict([
                    (ref[11:], b.node(refs[ref]))
                        for ref in refs.keys()
                            if ref.startswith('refs/heads/')])
                return stripped_refs
        return {}

    def pushkey(self, namespace, key, old, new):
        # Only delete if we are given a '' or null revision, and if we find the git_handler discussed above.
        if hasattr(self, 'git_handler') and (new == GitHandler.null_revision or new == ''):
            client, path = self.git_handler.get_transport_and_path(self.path)
            def changed(refs):
                desired = gitrepo.head_prefix + key
                if desired in refs:
                    refs = dict(refs) # As of 0.8.5, dulwich expects a NEW dictionary to be returned
                    refs[desired] = ZERO_SHA # git interprets "set to zero hash" as "delete"
                return refs

            genpack = self.git_handler.git.object_store.generate_pack_contents
            try:
                self.git_handler.ui.status(_("creating and sending data for remote bookmark deletion\n"))
                changed_refs = client.send_pack(path, changed, genpack)
            except (HangupException, GitProtocolError), e:
                raise hgutil.Abort(_("git remote error: ") + str(e))
        return True

    # used by incoming in hg <= 1.6
    def branches(self, nodes):
        return []

gitrepo.head_prefix = "refs/heads/"

instance = gitrepo

def islocal(path):
    u = util.url(path)
    return not u.scheme or u.scheme == 'file'
