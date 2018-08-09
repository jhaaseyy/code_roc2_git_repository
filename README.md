# ROC2 CODES
This package contains the C codes for the ROC and Zephyr modules for the Strateole2 Project.
Strateole2 is a NSF funded project that collects GPS data aboard a weather balloon to study the atmosphere, known as radio occultation (ROC)

## Getting Started

### First Time Git Users: Configure your computer for GIT 
(This is primarily for Jennifer to manage the repository on the lab computers)
1. Open Terminal and Install Git
```
$ sudo apt-get install git
```
2. Create Git user profile
```
$ git config --global user.name "yourusername"
$ git config --global user.email "user@gmail.com"
```
3. Head to the file directory you want the Git files to be downloaded to
```
$ cd /directory/folder
example:
$ cd ~/Dropbox/work/strateole2_private/docs_system/roc_software_design/code_roc2_git/
```
4. Clone the Github Repository into the folder
```
$ git clone https://github.com/haaseresearch/code_roc2_git.git
```
5. Now you have a complete copy of the ROC2 repository on your local machine. You may list all the items.
```
$ ls
```

### Typical Workflow and Common Git Commands
Modify this to include the procedure for choosing the branch, provide instructions for how to do this here.
1. At the start of each day or work session, always do a git pull to get the latest version of the repo. Remember to always cd into your Git folder.
```
$ cd /directory/folder
example:
$ cd /Users/jhaase/Dropbox/work/strateole2_private/docs_system/roc_software_design/code_roc2/temp
if you need to ...
$ git clone https://github.com/jhaaseyy/code_roc2_git_repository.git
$ cd code_roc2_git_repository
$ git pull
```
2. Always start a new branch and avoid tampering with the master branch. Use the conventional naming convention "roc2.x_code_yyyymmdd_vw". This helps tracking of version numbers.
```
$ git branch roc2.x_code_yyyymmdd_vw
```
3. You can check all the existing branches
```
$ git branch
```
4. To switch to the new branch and check
```
$ git checkout roc2.x_code_yyyymmdd_vw
$ git branch
```
5. Do what you need to do to edit the codes or add new files.. etc
   Note: Plan ahead with team and avoid changing same part of the codes simultaneously, or there will be merging conflicts
6. When you're ready to commit the changes and push the branch them up to the Github repo
```
$ git add -A //This adds all the changes 
$ git commit -m "comments here" //This commits the changes with comment
$ git push --set-upstream origin roc2.2_code_20180722_026 //This pushes the commit onto the repo
```
8. On Github, do a pull request and comment your changes
9. An authorized collaborator or admin will review the changes and merge the branch into the main branch

### ROC Version and Branch Control
1. Each version of the code will have its own branch and shall be named with the convention: roc2.x_code_yyyymmdd_vw. eg:roc2.2_code_20180711. Default branch should always be the latest version.
2. (For Repository Owner only) To change default branch, go [here] (https://github.com/jhaaseyy/code_roc2_git_repository/settings/branches) and change default branch to the latest version.
3. To delete a branch locally
```
$ git branch -d <branch_name>
```

## Installation on Hardware

Download a copy of the folders here, including the makefile into the ROC linux computer. We recommend placing this at the root directory.
```
$ cd /
```
Go to the make folder and compile using make
```
$ cd /root/roc/roc
$ make
```

## Contributors
* Principal Researcher: Dr.Jennifer Haase
* Post-Doc: Bing Cao
* Primary Engineer: Dave Jabson
* Research Assistant: Steven Liang, Jimmy Lozano

## License
See [LICENSE.md](https://github.com/jhaaseyy/code_roc2_git_repository/blob/master/LICENSE) for details
