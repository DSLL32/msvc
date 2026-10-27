// For comments on this program - see file 'readme'

#include <stdio.h>
#define ZZmain
#include "data.h"
#include "data.cc"
// --------------------------------------------------------
#define BSIZE 80
int main(void){
    void addRoute(char *name1, char *name2, int id, int dist);
    void findRoute(char *town1,char *town2);
    char buff[BSIZE],cmd[BSIZE],town1[BSIZE],town2[BSIZE];
    static char *t[]={"Root"};
    char *v[1];
    FILE *fp;
    int id,dist;

    fp=fopen("backup","r"); // just test existence
    fclose(fp);
    if(fp){
        util.open("backup",1,v,t);
        root=(Root *)(v[0]);
    }
    else {
        root=new Root;
        hash.form(root,200);
    }
    
    printf("add [town1] [town2] [id] [dist];   route [town1] [town2];   exit\n>");
    while(fgets(buff,BSIZE,stdin)){
        sscanf(buff,"%s",cmd);
        if(!strcmp(cmd,"route")){
            sscanf(buff,"%s %s %s",cmd,town1,town2);
            findRoute(town1,town2);
        }
        else if(!strcmp(cmd,"add")){
            sscanf(buff,"%s %s %s %d %d",cmd,town1,town2,&id,&dist);
            addRoute(town1,town2,id,dist);
        }
        else if(!strcmp(cmd,"exit"))break;
        else printf("wrong command - try again ..\n");
        printf("add [town1] [town2] [id] [dist];   route [town1] [town2];   exit\n>");
    }
        
    v[0]=(char *)root;
    util.save("backup",1,v,t);
    return(0);
}
// --------------------------------------------------------
// add a new hwy between two towns 
void addRoute(char *name1, char *name2, int id, int dist){
    Town *t[2],*getTown(char *);
    Hwy *h;

    t[0]=getTown(name1);
    t[1]=getTown(name2);
    h=new Hwy(id,dist);
    netw.add(t,h);
}
// --------------------------------------------------------
// print a route, find total distance
void prtRoute(Town *t){
    int tot; Hwy *h; Town *tt[2];

    tot=0;
    if(!(t->route)){
        printf("no connection\n");
        return;
    }
    while(t){
        printf("%s",name.fwd(t));
        h=t->route;
        if(!h)break;
        printf("(%d) ",h->getID());
        netw.nodes(tt,h);
        if(t==tt[0])t=tt[1]; else t=tt[0];
        tot+=h->getDist();
    }
    printf(" dist=%d\n",tot);
}
// --------------------------------------------------------
// search for used Towns and initialize them
void clear(Town *t1){
    Town *t,*tt,*fw,*nt;
    Hwy *h;

    wave.add(root,t1);
    for(t=wave.child(root); ; t=nt){
        if(t->route||t==t1){
            netw_iterator ni(t);
            while(h=ni++){
                tt=ni.adj();
                if(tt->route){
                    fw=wave.fwd(tt);
                    if(!fw)wave.add(root,tt);
                }
            }
        }
        nt=wave.bwd(t);
        wave.del(root,t);
        t->route=NULL;
        t->best=INF;
        if(nt==t)break;
    }
}
// --------------------------------------------------------
// find a route and mark it by route pointers 
void search(Town *t2){
    Town *t,*tt,*fw,*nt;
    Hwy *h;
    int bot;

    bot=0;
    for(t=wave.child(root); ; t=nt){
        if(t==wave.child(root)){
            if(t2->route && t2->best<bot)return;
            bot=INF;
        }
        if(t->best<bot)bot=t->best;
        netw_iterator ni(t);
        while(h=ni++){
            tt=ni.adj();
            if(t->best+h->getDist()<tt->best){
                tt->route=h;
                tt->best=t->best+h->getDist();
                fw=wave.fwd(tt);
                if(!fw)wave.add(root,tt);
            }
        }
        nt=wave.bwd(t);
        wave.del(root,t);
        if(nt==t)break;
    }
}
// --------------------------------------------------------
// for given town names, find the route
void findRoute(char *town1,char *town2){
    Town *t1,*t2,*getTown(char *);

    t1=getTown(town1);
    t2=getTown(town2);
    if(t1==t2){printf("same point\n"); return;}
    wave.add(root,t1);
    t1->best=0;
    search(t2);
    prtRoute(t2);
    clear(t1);
}
// --------------------------------------------------------
// get a town by its name, create it if not found 
Town *getTown(char *nm){
    Town *t;
    static Town s; // for hashing 

    name.add(&s,nm);
    t=hash.get(root,&s);
    if(!t){ t=new Town(nm); hash.add(root,t);}
    nm=name.del(&s);
    return(t);
}
