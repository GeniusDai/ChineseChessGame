package com.dai;

import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher.Event;
import org.apache.zookeeper.data.ACL;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs.Ids;

import java.nio.channels.AcceptPendingException;
import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;
import java.util.Collections;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.net.Socket;

public class Executor implements Runnable, Watcher, AutoCloseable {
    static final byte END = '\n';
    static final byte SEP = '#';
    static final String red = "RED";
    static final String black = "BLACK";

    int socketPort;
    String zkAddr;

    static final String room = "/ROOM";
    static final String lockNode = "player";
    static final String ready = "ready";
    static final String leave = "leave";

    // Table info
    String player;
    String oppo;
    String color;

    boolean alive = true;

    ZooKeeper zk;
    Socket socket;
    InputStream in;
    OutputStream out;

    Executor(int port, String zkAddr) {
        this.socketPort = port;
        this.zkAddr = zkAddr;
        try {
            initSocketAndKeeper();
            Thread.sleep(2000);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    class InnerWatcher implements Watcher {
        CountDownLatch latch;
        InnerWatcher(CountDownLatch latch) {
            this.latch = latch;
        }

        @Override
        public void process(WatchedEvent event) {
            if (event.getState() == Event.KeeperState.SyncConnected) {
                System.out.println("ZooKeeper connected...");
                latch.countDown();
            }
        }
    }

    class ExecutorException extends Exception {
        public ExecutorException() {}

        public ExecutorException(String message) {
            super(message);
        }
    }

    void initSocketAndKeeper() throws IOException, InterruptedException, Exception {
        Socket socket = new Socket("localhost", socketPort);
        this.in = socket.getInputStream();
        this.out = socket.getOutputStream();

        CountDownLatch latch = new CountDownLatch(1);
        zk = new ZooKeeper(zkAddr, 3000, new InnerWatcher(latch));
        latch.await();
    }

    void selectTable() throws KeeperException, InterruptedException, ExecutorException {
        List<String> tables = zk.getChildren(room, null);
        for (String t : tables) {
            String fullTablePath = new File(room, t).toString();
            List<String> players = zk.getChildren(fullTablePath, null);
            for (String p : players) {
                String fullPlayerPath = new File(fullTablePath, p).toString();
                if (zk.getChildren(fullPlayerPath, null).size() == 0) {
                    System.out.println("trying to lock " + fullPlayerPath);
                    if (tryLock(fullPlayerPath, lockNode)) {
                        this.player = fullPlayerPath;
                        this.color = p;
                        switch(p) {
                        case red:
                            this.oppo = new File(fullTablePath, black).toString();
                            break;
                        case black:
                            this.oppo = new File(fullTablePath, red).toString();
                            break;
                        default:
                            throw new ExecutorException("player color unknown: " + p);
                        }
                        return;
                    }
                }
            }
        }
        throw new ExecutorException("Room is currently full of players");
    }

    void readyToPlay() throws KeeperException, InterruptedException{
        System.out.println("player: " + player);
        System.out.println("oppo: " + oppo);
        System.out.println("color: " + color);
        zk.setData(player, ready.getBytes(), -1);
        zk.exists(oppo, this);
        System.out.println("finish set ready");
    }

    String askForMove() throws IOException {
        System.out.println("First move will be asked");
        return moveThenAsk(red, "0 0 0 0");
    }

    String moveThenAsk(String color, String move) throws IOException {
        byte[] sep = new byte[1];
        sep[0] = SEP;
        String data = color + new String(sep) + move;
        System.out.println("sent data " + data);
        writeSocket(data);
        String answer = new String(readSocket());
        return answer;
    }

    boolean tryLock(String path, String pattern) throws KeeperException, InterruptedException {
        String fullLock = zk.create(new File(path, pattern).toString(), null, 
            Ids.OPEN_ACL_UNSAFE, CreateMode.EPHEMERAL_SEQUENTIAL);
        List<String> locks = zk.getChildren(path, null);
        Collections.sort(locks);
        System.out.println("lock: " + fullLock);
        if (fullLock.endsWith(locks.get(0))) {
            System.out.println("lock: " + fullLock + " succeed!");
            return true;
        }
        zk.delete(fullLock, -1);
        return false;
    }

    byte[] readSocket() throws IOException {
        byte[] bufffer = new byte[128];
        byte[] curr = new byte[1];
        int i = 0;
        while(true) {
            in.read(curr);
            if (curr[0] != END) {
                bufffer[i++] = curr[0];
            } else {
                break;
            }
        }
        byte[] result = new byte[i];
        System.arraycopy(bufffer, 0, result, 0, i);
        System.out.println("receive data from socket: " + new String(result));
        return result;
    }

    void writeSocket(byte[] data) throws IOException {
        out.write(data);
        out.write(END);
    }

    void writeSocket(String data) throws IOException{
        writeSocket(data.getBytes());
    }

    void setThenWatch(byte[] data) throws KeeperException, InterruptedException {
        zk.setData(player, data, -1);
        zk.exists(oppo, this);
    }

    @Override
    public void run() {
        System.out.println("thread started...");
        try {
            selectTable();
            readyToPlay();
            synchronized (this) {
                while (alive) {
                    wait();
                }
            }
        } catch (KeeperException | InterruptedException | ExecutorException e) {
            e.printStackTrace();
        }
        System.out.println("thread stopped...");
    }

    @Override
    public void process(WatchedEvent event) {
        System.out.println("process triggered by " + event.toString());
        assert(event.getPath().equals(oppo));
        try {
            String data = new String(zk.getData(oppo, null, null));
            System.out.println("data: " + data);
            switch(data) {
            case ready:
                if (color.equals(red)) {
                    setThenWatch(askForMove().getBytes());
                }
                break;
            case leave:
                close();
                break;
            default:
                setThenWatch(moveThenAsk(color, data).getBytes());
            }
        } catch(Exception e){
            e.printStackTrace();
            close();
        }

    }

    @Override
    public void close() {
        try {
            if (this.socket != null) {
                this.socket.close();
            }
            zk.setData(player, leave.getBytes(), -1);
            synchronized (this) {
                alive = false;
                notifyAll();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) throws Exception {
        assert(args.length == 2);
        Runnable runnable = new Executor(Integer.parseInt(args[0]), args[1]);
        new Thread(runnable).start();
    }
}
