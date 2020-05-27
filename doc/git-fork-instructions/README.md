## Cheat Sheet
after you clone your fork do this (read below for explanation):

```
git remote add sst-official https://github.com/sstsimulator/sst-core
git pull --all
git branch devel
git branch devel --set-upstream-to sst-official/devel
```

## Creating a Fork
A fork is simply a personal copy of the repo on github for example the official SST-core repo is located here:

`https://github.com/sstsimulator/sst-core`

my fork of that repo is here:

`https://github.com/fryeguy52/sst-core`



The way to create your own fork is to click the fork button in the upper right while you are at the official repo


## Using your fork
Immediately after you have created your fork the `devel` and `master` branches will be the same as on the official repo, so you can simply clone your fork and work from that clone

```
git clone https://github.com/fryeguy52/sst-core
```

then create a brach off of `devel` make your changes, commit and push. All the commits will be on a brach on your fork.

```
git checkout devel
git checkout -b my-new-feature-branch
<making changes ...>
git add <approprite files>
git commit
git push
```

once you are done with the feature and would like to get changes to the official repo, you will do that with a pull request from the branch on your fork to the `devel` branch on the official repo. This is done through the github UI.


## Getting updates from the official repo to your local clone

Your fork will not automatically get updates from the official repo but you will want to regularly get those updates, especially as you are starting a new branch. The easiest way to do this is to tell the `devel` branch in your local repo to track the `devel` branch on the official repo. First create a new remote, then point the devel branch to that remot
e.

```
git remote add sst-official https://github.com/sstsimulator/sst-core
git pull --all
git branch devel
git branch devel --set-upstream-to sst-official/devel
```
You can verify that things are setup correctly

```
git remote -vv
origin	   https://github.com/fryeguy52/sst-core (fetch)
origin	   https://github.com/fryeguy52/sst-core (push)
sst-official					 https://github.com/sstsimulator/sst-core.git (fetch)
sst-official					 https://github.com/sstsimulator/sst-core.git (push)

git branch -vv
  devel  be1b790 [sst-official/devel: ahead 82, behind 61] Merge pull request #496 from sstsimulator/devel
* master be1b790 [origin/master] Merge pull request #496 from sstsimulator/devel
```
you should see that the `sst-official` is pointing to the official repo and that the `devel` branch in you local repo is pointing to `sst-officiall/devel`. Now all you need to do to get updates from the official sst-core repo devel branch is:

```
git checkout devel
git pull
```

Now you can branch from `devel` to create new features and when you push those feature branches they will push to your fork. When you pull devel, it will get updates from the official sst repo