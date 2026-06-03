import QtQuick
import QtQuick.Controls

Component {
    id: repoListDelegate
    RepoListItem {
        repoName: model.name
        repoPath: model.path
        repoType: model.type
        isSelected: model.isSelected
        onItemClicked: selectRepo(index)
        onRemoveClicked: removeRepo(index)
        onRenameClicked: {
            renameRepoDialog.index = index
            renameRepoDialog.oldName = model.name
            renameRepoDialog.newNameField.text = model.name
            renameRepoDialog.open()
        }
        onEditUrlClicked: {
            editRepoDialog.index = index
            editRepoDialog.name = model.name
            editRepoDialog.urlField.text = model.url
            editRepoDialog.open()
        }
    }
}
