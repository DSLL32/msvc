//! Bounded receipts for explicit PE same-image child capture, not a provider.
use anyhow::{Context, Result, ensure};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Owner {
    pub pid: u32,
    pub tid: u32,
    pub base: u32,
    pub child: bool,
    pub entry: bool,
}
impl Owner {
    pub fn decode(bytes: &[u8], registers: usize) -> Result<(Self, Vec<u8>)> {
        ensure!(bytes.len() == 24 + registers, "PE SELF event size differs");
        let words: Vec<u32> = bytes[..24]
            .chunks_exact(4)
            .map(|v| u32::from_le_bytes(v.try_into().unwrap()))
            .collect();
        ensure!(
            words[0] == 1 && words[1..4].iter().all(|v| *v != 0) && words[4] <= 1 && words[5] <= 1,
            "invalid PE SELF owner"
        );
        Ok((
            Self {
                pid: words[1],
                tid: words[2],
                base: words[3],
                child: words[4] == 1,
                entry: words[5] == 1,
            },
            bytes[24..].to_vec(),
        ))
    }
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Process {
    pub pid: u32,
    pub tid: u32,
    pub base: u32,
    pub child: bool,
    pub entry_observed: bool,
    pub exit_status: u32,
    pub terminated: bool,
    pub signaled: bool,
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Capture {
    pub root_pid: u32,
    pub frontier_pid: Option<u32>,
    pub processes: Vec<Process>,
}
impl Capture {
    pub fn validate(&self) -> Result<()> {
        ensure!(
            (1..=2).contains(&self.processes.len()),
            "PE SELF process bound differs"
        );
        let root = &self.processes[0];
        ensure!(
            root.pid == self.root_pid && !root.child,
            "PE SELF root differs"
        );
        let mut seen = std::collections::BTreeSet::new();
        for (index, process) in self.processes.iter().enumerate() {
            ensure!(
                process.pid != 0
                    && process.tid != 0
                    && process.base != 0
                    && seen.insert(process.pid)
                    && process.child == (index == 1)
                    && process.entry_observed
                    && process.signaled,
                "invalid PE SELF process receipt"
            );
            ensure!(
                !process.terminated || process.exit_status == 125,
                "PE SELF forced termination status differs"
            );
            ensure!(
                process.terminated
                    == self.frontier_pid.is_some_and(
                        |pid| pid == process.pid || (!process.child && pid != self.root_pid)
                    ),
                "PE SELF teardown differs from actual frontier"
            );
        }
        ensure!(
            self.frontier_pid.is_none_or(|pid| seen.contains(&pid)),
            "unknown PE SELF frontier owner"
        );
        Ok(())
    }
    pub fn check_children(&self, actual: &[crate::self_child::ChildResult]) -> Result<()> {
        self.validate()?;
        ensure!(
            self.frontier_pid.is_none_or(|pid| pid == self.root_pid),
            "PE SELF child frontier requires standalone child qualification before a combined pair"
        );
        let children: Vec<_> = self.processes.iter().filter(|p| p.child).collect();
        ensure!(
            actual.len() == children.len(),
            "PE SELF child count differs"
        );
        for (expected, observed) in children.iter().zip(actual) {
            ensure!(
                !expected.terminated
                    && observed.errors.is_empty()
                    && observed.logical_exit_code == Some(expected.exit_status)
                    && observed.trap_rva.is_none(),
                "PE SELF child terminal differs"
            );
        }
        Ok(())
    }
}
#[derive(Default)]
pub struct Tracker {
    pub current: Option<Owner>,
    root: u32,
    owners: BTreeMap<u32, Owner>,
    exits: BTreeMap<u32, u32>,
}
impl Tracker {
    pub fn observe(&mut self, owner: Owner, kind: u32, status: u32) -> Result<()> {
        if self.root == 0 {
            ensure!(!owner.child, "PE SELF initial process is not root");
            self.root = owner.pid;
        }
        ensure!(
            owner.child == (owner.pid != self.root),
            "PE SELF owner role changed"
        );
        if let Some(old) = self.owners.get(&owner.pid) {
            ensure!(
                old.tid == owner.tid
                    && old.base == owner.base
                    && old.child == owner.child
                    && (!old.entry || owner.entry),
                "PE SELF owner identity changed"
            );
        }
        self.owners.insert(owner.pid, owner.clone());
        ensure!(self.owners.len() <= 2, "PE SELF process limit exceeded");
        if kind == 0 || kind == 5 {
            ensure!(
                (kind == 5) == owner.child
                    && owner.entry
                    && self.exits.insert(owner.pid, status).is_none(),
                "invalid PE SELF exit event"
            );
        }
        self.current = Some(owner);
        Ok(())
    }
    pub fn finish(&self, diagnostics: &str, frontier: bool) -> Result<Capture> {
        let mut processes = Vec::new();
        for line in diagnostics
            .lines()
            .filter_map(|v| v.strip_prefix("pe-self-process "))
        {
            let values: Vec<u32> = line
                .split_ascii_whitespace()
                .map(|v| v.parse().context("PE SELF receipt integer"))
                .collect::<Result<_>>()?;
            ensure!(
                values.len() == 9
                    && values[3..6].iter().all(|v| *v <= 1)
                    && values[7..9].iter().all(|v| *v <= 1)
                    && values[5] == 1,
                "invalid PE SELF cleanup receipt"
            );
            let owner = self
                .owners
                .get(&values[0])
                .context("unobserved PE SELF process")?;
            ensure!(
                owner.tid == values[1]
                    && owner.base == values[2]
                    && owner.child == (values[3] == 1)
                    && owner.entry == (values[4] == 1),
                "PE SELF cleanup owner differs"
            );
            if values[7] == 0 {
                ensure!(
                    self.exits.get(&owner.pid) == Some(&values[6]),
                    "PE SELF natural exit differs"
                );
            }
            processes.push(Process {
                pid: values[0],
                tid: values[1],
                base: values[2],
                child: values[3] == 1,
                entry_observed: values[4] == 1,
                exit_status: values[6],
                terminated: values[7] == 1,
                signaled: values[8] == 1,
            });
        }
        ensure!(
            processes.len() == self.owners.len(),
            "missing PE SELF cleanup receipts"
        );
        let capture = Capture {
            root_pid: self.root,
            frontier_pid: if frontier {
                Some(self.current.as_ref().context("PE SELF frontier owner")?.pid)
            } else {
                None
            },
            processes,
        };
        capture.validate()?;
        Ok(capture)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn owner(pid: u32, child: bool, base: u32) -> Owner {
        Owner {
            pid,
            tid: pid + 1,
            base,
            child,
            entry: true,
        }
    }
    #[test]
    fn opt_in_frame_preserves_registers_and_distinct_process_bases() {
        let mut data = Vec::new();
        for word in [1u32, 24, 25, 0x500000, 1, 1] {
            data.extend(word.to_le_bytes());
        }
        data.extend([9u8; 152]);
        let (decoded, registers) = Owner::decode(&data, 152).unwrap();
        assert_eq!(decoded, owner(24, true, 0x500000));
        assert_eq!(registers, vec![9; 152]);
        for size in [0, 151, 153] {
            assert!(Owner::decode(&data, size).is_err());
        }
        for (offset, word) in [(0, 2u32), (4, 0), (12, 0), (16, 2), (20, 2)] {
            let mut bad = data.clone();
            bad[offset..offset + 4].copy_from_slice(&word.to_le_bytes());
            assert!(Owner::decode(&bad, 152).is_err());
        }
    }
    #[test]
    fn natural_child_status_and_frontier_cleanup_are_distinct() {
        let mut tracker = Tracker::default();
        tracker.observe(owner(10, false, 0x400000), 1, 0).unwrap();
        tracker.observe(owner(20, true, 0x500000), 1, 0).unwrap();
        tracker.observe(owner(20, true, 0x500000), 5, 259).unwrap();
        tracker.observe(owner(10, false, 0x400000), 1, 0).unwrap();
        let receipt = tracker.finish("pe-self-process 10 11 4194304 0 1 1 125 1 1\npe-self-process 20 21 5242880 1 1 1 259 0 1\n", true).unwrap();
        assert_eq!(receipt.frontier_pid, Some(10));
        assert_eq!(receipt.processes[1].exit_status, 259);
        let raw = serde_json::to_vec(&receipt).unwrap();
        assert_eq!(serde_json::from_slice::<Capture>(&raw).unwrap(), receipt);
        assert!(
            tracker
                .finish("pe-self-process 10 11 4194304 0 1 1 125 1 1\n", true)
                .is_err()
        );
        assert!(tracker.finish("pe-self-process 10 11 4194304 0 1 1 125 1 0\npe-self-process 20 21 5242880 1 1 1 0 0 1\n", true).is_err());
    }
    #[test]
    fn changed_unknown_and_excess_owners_are_rejected() {
        let mut tracker = Tracker::default();
        assert!(tracker.observe(owner(20, true, 0x500000), 1, 0).is_err());
        tracker.observe(owner(10, false, 0x400000), 1, 0).unwrap();
        assert!(tracker.observe(owner(10, false, 0x600000), 1, 0).is_err());
        assert!(tracker.observe(owner(10, true, 0x400000), 1, 0).is_err());
        tracker.observe(owner(20, true, 0x500000), 1, 0).unwrap();
        assert!(tracker.observe(owner(30, true, 0x600000), 1, 0).is_err());
    }
    #[test]
    fn natural_child_count_and_full_status_must_match_native_receipt() {
        let proof = Capture {
            root_pid: 10,
            frontier_pid: None,
            processes: vec![
                Process {
                    pid: 10,
                    tid: 11,
                    base: 0x400000,
                    child: false,
                    entry_observed: true,
                    exit_status: 0,
                    terminated: false,
                    signaled: true,
                },
                Process {
                    pid: 20,
                    tid: 21,
                    base: 0x500000,
                    child: true,
                    entry_observed: true,
                    exit_status: 7,
                    terminated: false,
                    signaled: true,
                },
            ],
        };
        let mut actual = vec![crate::self_child::ChildResult {
            launcher_pid: 100,
            parent_pid: 99,
            executor_pid: 101,
            kernel_status: 7 << 8,
            terminal: "exit:7".into(),
            logical_exit_code: Some(7),
            trap_rva: None,
            trap_site: None,
            errors: vec![],
        }];
        assert!(proof.check_children(&actual).is_ok());
        actual[0].logical_exit_code = Some(0x12340007);
        assert!(proof.check_children(&actual).is_err());
        assert!(proof.check_children(&[]).is_err());
    }
    #[test]
    fn child_pending_frontier_is_not_claimed_as_combined_native_acceptance() {
        let mut tracker = Tracker::default();
        tracker.observe(owner(10, false, 0x400000), 1, 0).unwrap();
        tracker
            .observe(owner(20, true, 0x500000), 2, 0x1200)
            .unwrap();
        let receipt=tracker.finish("pe-self-process 10 11 4194304 0 1 1 125 1 1\npe-self-process 20 21 5242880 1 1 1 125 1 1\n",true).unwrap();
        assert_eq!(receipt.frontier_pid, Some(20));
        assert!(receipt.check_children(&[]).is_err());
    }
}
