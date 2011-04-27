/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Gary W. Ng
**********/

#include "ngspice.h"
#include "swdefs.h"
#include "cktdefs.h"
#include "iferrmsg.h"
#include "noisedef.h"
#include "suffix.h"

/*
 * SWnoise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with voltage- controlled switches.  It starts with the
 *    model *firstModel and traverses all of its instances.  It then
 *    proceeds to any other models on the linked list.  The total output
 *    noise density generated by the SW's is summed in the variable
 *    "OnDens".
 */


int
SWnoise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt, Ndata *data, double *OnDens)
{
    SWmodel *firstModel = (SWmodel *) genmodel;
    SWmodel *model;
    SWinstance *inst;
    char name[N_MXVLNTH];
    double tempOutNoise;
    double tempInNoise;
    double noizDens;
    double lnNdens;
    int current_state;


    for (model=firstModel; model != NULL; model=model->SWnextModel) {
	for (inst=model->SWinstances; inst != NULL; inst=inst->SWnextInstance) {
	    if (inst->SWowner != ARCHme) continue;

	    switch (operation) {

	    case N_OPEN:

		/* see if we have to to produce a summary report */
		/* if so, name the noise generator */

		if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
		    switch (mode) {

		    case N_DENS:
			(void)sprintf(name,"onoise_%s",inst->SWname);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			break;

		    case INT_NOIZ:
			(void)sprintf(name,"onoise_total_%s",inst->SWname);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			(void)sprintf(name,"inoise_total_%s",inst->SWname);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			break;
		    }
		}
		break;

	    case N_CALC:
		switch (mode) {

		case N_DENS:
		    current_state = (int)*(ckt->CKTstate0 + inst->SWstate);
		    NevalSrc(&noizDens,&lnNdens,ckt,THERMNOISE,
				 inst->SWposNode,inst->SWnegNode,
				 current_state?(model->SWonConduct):(model->SWoffConduct));

		    *OnDens += noizDens;

		    if (data->delFreq == 0.0) { 

			/* if we haven't done any previous integration, we need to */
			/* initialize our "history" variables                      */

			inst->SWnVar[LNLSTDENS] = lnNdens;

			/* clear out our integration variable if it's the first pass */

			if (data->freq == ((NOISEAN*)ckt->CKTcurJob)->NstartFreq) {
			    inst->SWnVar[OUTNOIZ] = 0.0;
			}
		    } else {   /* data->delFreq != 0.0 (we have to integrate) */
			tempOutNoise = Nintegrate(noizDens, lnNdens,
			       inst->SWnVar[LNLSTDENS], data);
			tempInNoise = Nintegrate(noizDens * 
			       data->GainSqInv ,lnNdens + data->lnGainInv,
			       inst->SWnVar[LNLSTDENS] + data->lnGainInv,
			       data);
			inst->SWnVar[OUTNOIZ] += tempOutNoise;
			inst->SWnVar[INNOIZ] += tempInNoise;
			data->outNoiz += tempOutNoise;
			data->inNoise += tempInNoise;
			inst->SWnVar[LNLSTDENS] = lnNdens;
		    }
		    if (data->prtSummary) {
			data->outpVector[data->outNumber++] = noizDens;
		    }
		    break;

		case INT_NOIZ:        /* already calculated, just output */
		    if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
			data->outpVector[data->outNumber++] = inst->SWnVar[OUTNOIZ];
			data->outpVector[data->outNumber++] = inst->SWnVar[INNOIZ];
		    }    /* if */
		    break;
		}    /* switch (mode) */
		break;

	    case N_CLOSE:
		return (OK);         /* do nothing, the main calling routine will close */
		break;               /* the plots */
	    }    /* switch (operation) */
	}    /* for inst */
    }    /* for model */

return(OK);
}
