# Cheat Sheet

## after you clone your fork do this

```
git remote add sst-official https://github.com/sstsimulator/sst-core
git pull --all
git branch devel
git branch devel --set-upstream-to sst-official/devel
```

## When you make a new branch do this:
```
git checkout devel
git pull
git checkout -b <YOUR_BRANCH_NAME>
git push --set-upstream origin <YOUR_BRANCH_NAME>
```
