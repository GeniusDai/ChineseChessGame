package com.dai;

import java.io.IOException;
import java.util.concurrent.CountDownLatch;

import com.dai.Executor;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.ZooDefs.Ids;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooDefs.Ids;
import java.io.File;

public class InitializeServer implements Watcher {
    static final CountDownLatch latch = new CountDownLatch(1);
    String table = "TABLE";
    int baseNumber = 10000;

    InitializeServer(String zkAddr, int roomNumber) {
        ZooKeeper zk;
        try {
            zk = new ZooKeeper(zkAddr, 3000, this);
            latch.await();
            zk.create(Executor.room, null, Ids.OPEN_ACL_UNSAFE, CreateMode.CONTAINER);
            for (int i = 0; i < roomNumber; ++i) {
                String tableSeq = table + Integer.toString(baseNumber + i);
                String fullTablePath = new File(Executor.room, tableSeq).toString();
                zk.create(fullTablePath, null, Ids.OPEN_ACL_UNSAFE, CreateMode.CONTAINER);
                zk.create(new File(fullTablePath, Executor.red).toString(), null, Ids.OPEN_ACL_UNSAFE, CreateMode.CONTAINER);
                zk.create(new File(fullTablePath, Executor.black).toString(), null, Ids.OPEN_ACL_UNSAFE, CreateMode.CONTAINER);
            }
        } catch (IOException | InterruptedException | KeeperException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void process(WatchedEvent event) {
        latch.countDown();
    }

    public static void main(String[] args) {
        InitializeServer init = new InitializeServer(args[0], 10);
    }
}
